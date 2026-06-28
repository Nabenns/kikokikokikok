import { stringify } from "smol-toml";

/**
 * Per-tenant GrowServer config.toml generation.
 *
 * Key multi-tenant decision: the ENet game port INSIDE the container equals
 * the host port we allocated. Docker then maps hostPort/udp -> hostPort/udp
 * (same number both sides). This avoids the classic GTPS footgun where the
 * server thinks it listens on 17091 but clients are told a different port via
 * server_data — here they always match, so server_data can simply advertise
 * host_ip:gamePort and the client connects straight through.
 */

export interface TenantConfigInput {
  gamePort: number;
  /** Public address clients reach (the tenant subdomain). */
  address: string;
  /** Login/web frontend port exposed for this tenant (internal). */
  frontendPort?: number;
  maintenance?: { enable: boolean; message: string };
  bypassVersionCheck?: boolean;
  logLevel?: string;
}

/**
 * Mirrors the shape GrowServer's packages/config expects (config.toml).
 * We keep TLS paths pointing at the in-container generated certs.
 */
export function buildConfigToml(input: TenantConfigInput): string {
  const cfg = {
    web: {
      development: false,
      address: input.address,
      port: 443,
      ports: [input.gamePort],
      loginUrl: `${input.address}:${input.frontendPort ?? 8080}`,
      cdnUrl: "growserver-cache.netlify.app",
      maintenance: input.maintenance ?? {
        enable: false,
        message: "Server starting up...",
      },
      tls: {
        key: "./assets/ssl/server.key",
        cert: "./assets/ssl/server.crt",
      },
    },
    webFrontend: {
      root: "./.cache/website",
      port: input.frontendPort ?? 8080,
      tls: {
        key: "./.cache/ssl/_wildcard.growserver.app-key.pem",
        cert: "./.cache/ssl/_wildcard.growserver.app.pem",
      },
    },
    server: {
      bypassVersionCheck: input.bypassVersionCheck ?? true,
      logLevel: input.logLevel ?? "info",
    },
  };
  return stringify(cfg);
}

/**
 * The server_data.php payload a Growtopia client expects. The hosting proxy
 * serves this per-subdomain so clients are routed to the right instance.
 */
export function buildServerData(hostIp: string, gamePort: number): string {
  return [
    `server|${hostIp}`,
    `port|${gamePort}`,
    `type|1`,
    `#maint|`,
    `meta|gtps-host`,
    `RTENDMARKERBS1001`,
  ].join("\n");
}
