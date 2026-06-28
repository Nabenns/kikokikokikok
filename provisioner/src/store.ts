import Database from "better-sqlite3";
import { join } from "path";
import { mkdirSync } from "fs";

export type TenantStatus =
  | "provisioning"
  | "running"
  | "stopped"
  | "error"
  | "deleted";

export interface Tenant {
  id: string;
  name: string;
  subdomain: string;
  gamePort: number;
  containerId: string | null;
  status: TenantStatus;
  ownerId: string;
  plan: string;
  createdAt: number;
  updatedAt: number;
}

const DATA_DIR = process.env.GTPS_DATA_DIR || join(process.cwd(), "data");

export class Store {
  private db: Database.Database;

  constructor(dbPath?: string) {
    mkdirSync(DATA_DIR, { recursive: true });
    this.db = new Database(dbPath || join(DATA_DIR, "provisioner.db"));
    this.db.pragma("journal_mode = WAL");
    this.migrate();
  }

  private migrate() {
    this.db.exec(`
      CREATE TABLE IF NOT EXISTS tenants (
        id          TEXT PRIMARY KEY,
        name        TEXT NOT NULL,
        subdomain   TEXT NOT NULL UNIQUE,
        gamePort    INTEGER NOT NULL UNIQUE,
        containerId TEXT,
        status      TEXT NOT NULL,
        ownerId     TEXT NOT NULL,
        plan        TEXT NOT NULL DEFAULT 'free',
        createdAt   INTEGER NOT NULL,
        updatedAt   INTEGER NOT NULL
      );
      CREATE INDEX IF NOT EXISTS idx_tenants_owner ON tenants(ownerId);
      CREATE INDEX IF NOT EXISTS idx_tenants_status ON tenants(status);
    `);
  }

  insert(t: Tenant): Tenant {
    this.db
      .prepare(
        `INSERT INTO tenants (id,name,subdomain,gamePort,containerId,status,ownerId,plan,createdAt,updatedAt)
         VALUES (@id,@name,@subdomain,@gamePort,@containerId,@status,@ownerId,@plan,@createdAt,@updatedAt)`,
      )
      .run(t);
    return t;
  }

  update(id: string, patch: Partial<Tenant>): Tenant | undefined {
    const cur = this.get(id);
    if (!cur) return undefined;
    const next = { ...cur, ...patch, updatedAt: Date.now() };
    this.db
      .prepare(
        `UPDATE tenants SET name=@name,subdomain=@subdomain,gamePort=@gamePort,
         containerId=@containerId,status=@status,ownerId=@ownerId,plan=@plan,updatedAt=@updatedAt
         WHERE id=@id`,
      )
      .run(next);
    return next;
  }

  get(id: string): Tenant | undefined {
    return this.db.prepare(`SELECT * FROM tenants WHERE id = ?`).get(id) as
      | Tenant
      | undefined;
  }

  bySubdomain(sub: string): Tenant | undefined {
    return this.db
      .prepare(`SELECT * FROM tenants WHERE subdomain = ?`)
      .get(sub) as Tenant | undefined;
  }

  list(filter?: { ownerId?: string }): Tenant[] {
    if (filter?.ownerId) {
      return this.db
        .prepare(
          `SELECT * FROM tenants WHERE ownerId = ? AND status != 'deleted' ORDER BY createdAt DESC`,
        )
        .all(filter.ownerId) as Tenant[];
    }
    return this.db
      .prepare(`SELECT * FROM tenants WHERE status != 'deleted' ORDER BY createdAt DESC`)
      .all() as Tenant[];
  }

  usedPorts(): number[] {
    const rows = this.db
      .prepare(`SELECT gamePort FROM tenants WHERE status != 'deleted'`)
      .all() as { gamePort: number }[];
    return rows.map((r) => r.gamePort);
  }

  usedSubdomains(): string[] {
    const rows = this.db
      .prepare(`SELECT subdomain FROM tenants WHERE status != 'deleted'`)
      .all() as { subdomain: string }[];
    return rows.map((r) => r.subdomain);
  }

  close() {
    this.db.close();
  }
}
