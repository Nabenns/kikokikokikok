"use strict";

import fs from "fs/promises";
import { existsSync } from "fs";
import { buildItemsInfo } from "./item-info/build";
import { Database, dbDir, dbPath } from "@growserver/db";

async function setup() {
  const isData = existsSync(dbDir);
  if (!isData) {
    await fs.mkdir(dbDir);
    await fs.writeFile(dbPath, Buffer.alloc(0));
  }

  const db = new Database();
  await db.setup();
  // Item-wiki enrichment (growtopia.fandom.com) is OPTIONAL — it only adds
  // long-press item descriptions. The wiki is frequently unreachable / Forbidden
  // from server hosts (UND_ERR_CONNECT_TIMEOUT). A failure here must NOT abort
  // setup: if it does, the `server setup` script falls back to `nr bun`, which
  // isn't installed in our node:22 image -> `spawn bun ENOENT` -> the entrypoint
  // (set -e) kills the container -> restart loop. The ENet game server runs fine
  // without this file, so we degrade gracefully and continue.
  try {
    await buildItemsInfo();
  } catch (err) {
    console.warn(`[gtps-setup] item-info wiki enrichment skipped (non-fatal): ${err}`);
  }
  process.exit(0);
}

(async () => {
  await setup();
})();
