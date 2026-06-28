import Fastify from "fastify";
import cors from "@fastify/cors";
import { z } from "zod";
import { randomUUID } from "crypto";
import { Store, type Tenant } from "./store.js";
import { allocatePort, capacity, PortExhaustedError } from "./ports.js";
import { Orchestrator } from "./orchestrator.js";

/**
 * Provisioner HTTP API — the control plane the web dashboard talks to.
 *
 * Auth: a single shared bearer token (PROVISIONER_TOKEN). The web app is the
 * only client; end users never hit this directly. Keep it on a private network
 * / localhost in production.
 */

const TOKEN = process.env.PROVISIONER_TOKEN || "dev-provisioner-token";
const PORT = Number(process.env.PROVISIONER_PORT || 4500);
const HOST = process.env.PROVISIONER_HOST || "0.0.0.0";

const store = new Store();
const orchestrator = new Orchestrator();

const app = Fastify({ logger: { level: process.env.LOG_LEVEL || "info" } });
await app.register(cors, { origin: true });

// Bearer auth on everything except /health.
app.addHook("onRequest", async (req, reply) => {
  if (req.url === "/health") return;
  const auth = req.headers.authorization || "";
  if (auth !== `Bearer ${TOKEN}`) {
    reply.code(401).send({ ok: false, error: "unauthorized" });
  }
});

const SUBDOMAIN_RE = /^[a-z0-9]([a-z0-9-]{1,30}[a-z0-9])?$/;

const CreateTenantBody = z.object({
  name: z.string().min(1).max(64),
  subdomain: z
    .string()
    .toLowerCase()
    .refine((s) => SUBDOMAIN_RE.test(s), "subdomain invalid (a-z0-9-, 3-32 chars)"),
  ownerId: z.string().min(1),
  plan: z.string().default("free"),
});

app.get("/health", async () => ({ ok: true, capacity: capacity(), used: store.usedPorts().length }));

// ---- Create + provision a tenant ----
app.post("/tenants", async (req, reply) => {
  const parsed = CreateTenantBody.safeParse(req.body);
  if (!parsed.success) {
    return reply.code(400).send({ ok: false, error: parsed.error.issues[0]?.message });
  }
  const { name, subdomain, ownerId, plan } = parsed.data;

  if (store.bySubdomain(subdomain)) {
    return reply.code(409).send({ ok: false, error: "subdomain taken" });
  }

  let gamePort: number;
  try {
    gamePort = allocatePort(store.usedPorts());
  } catch (e) {
    if (e instanceof PortExhaustedError) {
      return reply.code(503).send({ ok: false, error: e.message });
    }
    throw e;
  }

  const now = Date.now();
  const tenant: Tenant = {
    id: randomUUID(),
    name,
    subdomain,
    gamePort,
    containerId: null,
    status: "provisioning",
    ownerId,
    plan,
    createdAt: now,
    updatedAt: now,
  };
  store.insert(tenant);

  try {
    const { containerId, serverData } = await orchestrator.provision(tenant);
    const updated = store.update(tenant.id, { containerId, status: "running" });
    return reply.code(201).send({ ok: true, tenant: updated, serverData });
  } catch (e) {
    store.update(tenant.id, { status: "error" });
    req.log.error({ err: e }, "provision failed");
    return reply.code(500).send({ ok: false, error: "provision failed", detail: String(e) });
  }
});

// ---- List tenants (optionally by owner) ----
app.get("/tenants", async (req) => {
  const ownerId = (req.query as { ownerId?: string }).ownerId;
  return { ok: true, tenants: store.list(ownerId ? { ownerId } : undefined) };
});

// ---- Get one tenant ----
app.get("/tenants/:id", async (req, reply) => {
  const { id } = req.params as { id: string };
  const t = store.get(id);
  if (!t) return reply.code(404).send({ ok: false, error: "not found" });
  return { ok: true, tenant: t };
});

// ---- Lifecycle actions ----
function withContainer(handler: (t: Tenant) => Promise<void>, nextStatus: Tenant["status"]) {
  return async (req: any, reply: any) => {
    const { id } = req.params as { id: string };
    const t = store.get(id);
    if (!t) return reply.code(404).send({ ok: false, error: "not found" });
    if (!t.containerId) return reply.code(409).send({ ok: false, error: "no container" });
    try {
      await handler(t);
      const updated = store.update(t.id, { status: nextStatus });
      return reply.send({ ok: true, tenant: updated });
    } catch (e) {
      req.log.error({ err: e }, "lifecycle action failed");
      return reply.code(500).send({ ok: false, error: String(e) });
    }
  };
}

app.post("/tenants/:id/stop", withContainer((t) => orchestrator.stop(t.containerId!), "stopped"));
app.post("/tenants/:id/start", withContainer((t) => orchestrator.start(t.containerId!), "running"));
app.post("/tenants/:id/restart", withContainer((t) => orchestrator.restart(t.containerId!), "running"));

// ---- Delete (remove container + mark deleted) ----
app.delete("/tenants/:id", async (req, reply) => {
  const { id } = req.params as { id: string };
  const t = store.get(id);
  if (!t) return reply.code(404).send({ ok: false, error: "not found" });
  if (t.containerId) await orchestrator.remove(t.containerId);
  const updated = store.update(t.id, { status: "deleted", containerId: null });
  return reply.send({ ok: true, tenant: updated });
});

// ---- Logs ----
app.get("/tenants/:id/logs", async (req, reply) => {
  const { id } = req.params as { id: string };
  const tail = Number((req.query as { tail?: string }).tail || 200);
  const t = store.get(id);
  if (!t) return reply.code(404).send({ ok: false, error: "not found" });
  if (!t.containerId) return reply.code(409).send({ ok: false, error: "no container" });
  const logs = await orchestrator.logs(t.containerId, tail);
  return { ok: true, logs };
});

app
  .listen({ port: PORT, host: HOST })
  .then(() => app.log.info(`provisioner listening on ${HOST}:${PORT}`))
  .catch((err) => {
    app.log.error(err);
    process.exit(1);
  });
