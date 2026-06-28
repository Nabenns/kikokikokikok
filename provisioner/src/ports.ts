/**
 * Port allocation for per-tenant GTPS instances.
 *
 * Each customer's GrowServer ENet game server needs its own UDP port on the
 * host. Growtopia clients reach it via the tenant subdomain's server_data,
 * which points at host_ip:gamePort. We hand out ports from a fixed range so
 * the host firewall only has to open one contiguous block.
 */

export const PORT_RANGE_START = Number(process.env.GTPS_PORT_START || 17091);
export const PORT_RANGE_END = Number(process.env.GTPS_PORT_END || 17190); // 100 slots

export class PortExhaustedError extends Error {
  constructor() {
    super(
      `No free game ports in range ${PORT_RANGE_START}-${PORT_RANGE_END}. ` +
        `Increase GTPS_PORT_END or add a host.`,
    );
    this.name = "PortExhaustedError";
  }
}

/**
 * Returns the lowest free port in [START, END] not present in `used`.
 * Deterministic (lowest-first) so allocation is stable and testable.
 */
export function allocatePort(used: number[]): number {
  const taken = new Set(used);
  for (let p = PORT_RANGE_START; p <= PORT_RANGE_END; p++) {
    if (!taken.has(p)) return p;
  }
  throw new PortExhaustedError();
}

export function capacity(): number {
  return PORT_RANGE_END - PORT_RANGE_START + 1;
}
