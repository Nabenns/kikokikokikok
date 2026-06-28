import { NextResponse } from "next/server";
import { execSync } from "child_process";

export const dynamic = "force-dynamic";

const CONTAINER = "gtps-gurotopia";
const DB_CONTAINER = "gurotopia-mariadb";
const DB_PASS = "ukEhT<ZM3~&t)jI{";
const DB_NAME = "gurotopia";

function run(cmd: string, timeout = 5): string {
  try {
    return execSync(cmd, { timeout: timeout * 1000, encoding: "utf-8" }).trim();
  } catch {
    return "";
  }
}

function getContainerStatus(name: string) {
  const raw = run(`docker ps -a --filter name=${name} --format "{{.Status}}|{{.Ports}}"`);
  if (!raw) return { running: false, status: "not found", ports: "" };
  const [status, ports] = raw.split("|");
  return { running: status.includes("Up"), status, ports };
}

function getUptime(): number {
  const raw = run(`docker ps --filter name=${CONTAINER} --format "{{.Status}}"`);
  const m = raw.match(/Up (\d+) (\w+)/);
  if (!m) return 0;
  const val = parseInt(m[1]);
  const unit = m[2];
  const mult: Record<string, number> = {
    second: 1, seconds: 1, minute: 60, minutes: 60,
    hour: 3600, hours: 3600, day: 86400, days: 86400,
  };
  return val * (mult[unit] || 0);
}

function getPlayerCount(): number {
  const raw = run(`docker exec ${CONTAINER} cat /proc/net/udp 2>/dev/null`);
  if (!raw) return 0;
  const lines = raw.trim().split("\n").slice(1);
  return lines.filter((l) => l.trim().split(/\s+/)[3] === "01").length;
}

function getServerData() {
  const raw = run(`curl -sk -X POST "https://127.0.0.1:8444/growtopia/server_data.php" -d "protocol=0&game_version=5.39&f=1" 2>/dev/null`);
  const data: Record<string, string> = {};
  for (const line of raw.split("\n")) {
    if (line.includes("|") && !line.startsWith("RTEND")) {
      const [k, v] = line.split("|", 2);
      data[k] = v;
    }
  }
  return data;
}

export function GET() {
  const data = {
    server: getContainerStatus(CONTAINER),
    mariadb: getContainerStatus(DB_CONTAINER),
    uptime_seconds: getUptime(),
    player_count: getPlayerCount(),
    server_data: getServerData(),
    timestamp: new Date().toISOString(),
  };
  return NextResponse.json(data);
}
