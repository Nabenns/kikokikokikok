import Docker from "dockerode";
import { writeFileSync, mkdirSync } from "fs";
import { join } from "path";
import postgres from "postgres";
import { buildConfigToml, buildServerData } from "./config.js";
import type { Tenant } from "./store.js";

/**
 * Container lifecycle for per-tenant GrowServer instances.
 *
 * Each tenant = one `gtps-growserver:latest` container:
 *   - its own Postgres DATABASE on the shared gtps-postgres (created on demand)
 *   - a generated config.toml bind-mounted onto /app/packages/config/config.toml
 *   - its ENet game UDP port published hostPort/udp -> hostPort/udp (same number
 *     both sides so server_data can advertise it directly)
 *   - attached to the shared `gtps_net` network so it reaches gtps-postgres /
 *     gtps-redis by container name
 */

const IMAGE = process.env.GTPS_IMAGE || "gtps-growserver:latest";
const NETWORK = process.env.GTPS_NETWORK || "gtps_net";
const PG_USER = process.env.GTPS_PG_USER || "growserver";
const PG_PASS = process.env.GTPS_PG_PASS || "gtps_local_dev";
// Admin DB used only to CREATE per-tenant databases.
const PG_ADMIN_DB = process.env.GTPS_PG_ADMIN_DB || "growserver";

// IMPORTANT: two different network vantage points for the same Postgres:
//   - the CONTAINER reaches it over the shared docker network by service name
//     (gtps-postgres:5432). This is what goes into the tenant's DATABASE_URL.
//   - the PROVISIONER process (running on the HOST, outside gtps_net) can't
//     resolve that name, so ensureDatabase() connects via the localhost-bound
//     debug port (127.0.0.1:5436). Keep these separate or one of them breaks.
const PG_CONTAINER_HOST = process.env.GTPS_PG_HOST || "gtps-postgres";
const PG_CONTAINER_PORT = Number(process.env.GTPS_PG_PORT || 5432);
const PG_ADMIN_HOST = process.env.GTPS_PG_ADMIN_HOST || "127.0.0.1";
const PG_ADMIN_PORT = Number(process.env.GTPS_PG_ADMIN_PORT || 5436);

// Host dir where generated per-tenant config files live (bind-mount source).
const CONFIG_DIR = process.env.GTPS_CONFIG_DIR || join(process.cwd(), "data", "configs");

// Per-tenant memory cap. GrowServer dev-mode runs 3 concurrent tsx-watch
// processes (server + logon + login-page) plus parses a ~16k-item items.dat,
// so 512MB OOM-kills it (confirmed via cgroup OOM). 1GB is the floor for
// dev-mode; a production build (pnpm build + start) would need less.
const MEM_LIMIT_MB = Number(process.env.GTPS_MEM_LIMIT_MB || 1024);

// Public host/IP that Growtopia clients ultimately reach (used in server_data).
const PUBLIC_HOST = process.env.GTPS_PUBLIC_HOST || "127.0.0.1";

export interface ProvisionResult {
  containerId: string;
  serverData: string;
}

export class Orchestrator {
  private docker: Docker;

  constructor(docker?: Docker) {
    this.docker = docker || new Docker(); // defaults to /var/run/docker.sock
    mkdirSync(CONFIG_DIR, { recursive: true });
  }

  /** Database name for a tenant — sanitized, deterministic. */
  private dbName(t: Tenant): string {
    return `tenant_${t.subdomain.replace(/[^a-z0-9_]/gi, "_").toLowerCase()}`;
  }

  private containerName(t: Tenant): string {
    return `gtps-tenant-${t.subdomain.replace(/[^a-z0-9_-]/gi, "-").toLowerCase()}`;
  }

  private tenantDatabaseUrl(t: Tenant): string {
    return `postgresql://${PG_USER}:${PG_PASS}@${PG_CONTAINER_HOST}:${PG_CONTAINER_PORT}/${this.dbName(t)}`;
  }

  /**
   * Create the tenant's Postgres database if it doesn't exist. CREATE DATABASE
   * can't be parameterized, so the name is sanitized + quoted. Idempotent.
   * Connects via the HOST-reachable admin endpoint (not the in-network name).
   */
  async ensureDatabase(t: Tenant): Promise<void> {
    const name = this.dbName(t);
    const admin = postgres({
      host: PG_ADMIN_HOST,
      port: PG_ADMIN_PORT,
      user: PG_USER,
      password: PG_PASS,
      database: PG_ADMIN_DB,
      max: 1,
    });
    try {
      const rows = await admin`SELECT 1 FROM pg_database WHERE datname = ${name}`;
      if (rows.length === 0) {
        // name is sanitized to [a-z0-9_]; safe to inline as a quoted identifier.
        await admin.unsafe(`CREATE DATABASE "${name}"`);
      }
    } finally {
      await admin.end({ timeout: 5 });
    }
  }

  /** Write the per-tenant config.toml to the host bind-mount source dir. */
  private writeConfig(t: Tenant): string {
    const toml = buildConfigToml({
      gamePort: t.gamePort,
      address: `${t.subdomain}.${PUBLIC_HOST}`,
    });
    const path = join(CONFIG_DIR, `${t.id}.config.toml`);
    writeFileSync(path, toml, "utf-8");
    return path;
  }

  /**
   * Provision (create + start) a tenant container. Returns container id and the
   * server_data payload to serve for this tenant's subdomain.
   */
  async provision(t: Tenant): Promise<ProvisionResult> {
    await this.ensureDatabase(t);
    const configPath = this.writeConfig(t);
    const name = this.containerName(t);

    // Remove any stale container with the same name (idempotent re-provision).
    await this.removeIfExists(name);

    const udp = `${t.gamePort}/udp`;
    const container = await this.docker.createContainer({
      name,
      Image: IMAGE,
      Env: [`DATABASE_URL=${this.tenantDatabaseUrl(t)}`],
      ExposedPorts: { [udp]: {} },
      HostConfig: {
        NetworkMode: NETWORK,
        RestartPolicy: { Name: "unless-stopped" },
        PortBindings: {
          [udp]: [{ HostPort: String(t.gamePort) }],
        },
        Binds: [`${configPath}:/app/packages/config/config.toml:ro`],
        Memory: MEM_LIMIT_MB * 1024 * 1024,
      },
    });
    await container.start();

    return {
      containerId: container.id,
      serverData: buildServerData(PUBLIC_HOST, t.gamePort),
    };
  }

  async stop(containerId: string): Promise<void> {
    const c = this.docker.getContainer(containerId);
    await c.stop({ t: 10 }).catch(() => {});
  }

  async start(containerId: string): Promise<void> {
    await this.docker.getContainer(containerId).start();
  }

  async restart(containerId: string): Promise<void> {
    await this.docker.getContainer(containerId).restart({ t: 10 });
  }

  async remove(containerId: string): Promise<void> {
    const c = this.docker.getContainer(containerId);
    await c.remove({ force: true }).catch(() => {});
  }

  private async removeIfExists(name: string): Promise<void> {
    const list = await this.docker.listContainers({
      all: true,
      filters: { name: [name] },
    });
    for (const info of list) {
      await this.docker.getContainer(info.Id).remove({ force: true }).catch(() => {});
    }
  }

  /** Recent logs for a container (combined stdout+stderr, demuxed to text). */
  async logs(containerId: string, tail = 200): Promise<string> {
    const c = this.docker.getContainer(containerId);
    const buf = (await c.logs({
      stdout: true,
      stderr: true,
      tail,
      timestamps: false,
    })) as unknown as Buffer;
    // Strip Docker's 8-byte stream multiplexing header from each frame.
    return stripDockerHeader(buf);
  }
}

/**
 * Docker multiplexes stdout/stderr with an 8-byte header per frame when no TTY.
 * Strip it so logs are human-readable. Tolerates already-clean buffers.
 */
function stripDockerHeader(buf: Buffer): string {
  const out: Buffer[] = [];
  let i = 0;
  while (i < buf.length) {
    // header: [stream(1)][000][size(4 BE)]
    if (i + 8 <= buf.length && (buf[i] === 1 || buf[i] === 2) && buf[i + 1] === 0) {
      const len = buf.readUInt32BE(i + 4);
      out.push(buf.subarray(i + 8, i + 8 + len));
      i += 8 + len;
    } else {
      out.push(buf.subarray(i));
      break;
    }
  }
  return Buffer.concat(out).toString("utf-8");
}
