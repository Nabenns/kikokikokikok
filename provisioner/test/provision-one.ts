/**
 * E2E de-risk: provision ONE real tenant container via the orchestrator,
 * then the caller checks logs + UDP bind. Run: tsx test/provision-one.ts
 */
import { Orchestrator } from "../src/orchestrator.js";
import type { Tenant } from "../src/store.js";

const now = Date.now();
const tenant: Tenant = {
  id: "test-" + now,
  name: "Smoke Test Server",
  subdomain: "smoke" + (now % 100000),
  gamePort: 17091,
  containerId: null,
  status: "provisioning",
  ownerId: "tester",
  plan: "free",
  createdAt: now,
  updatedAt: now,
};

const orch = new Orchestrator();
console.log(`provisioning tenant ${tenant.subdomain} on port ${tenant.gamePort} ...`);
const res = await orch.provision(tenant);
console.log("containerId:", res.containerId);
console.log("server_data:\n" + res.serverData);
console.log("\n--- container started; poll logs separately ---");
process.exit(0);
