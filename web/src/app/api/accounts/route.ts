import { NextResponse } from "next/server";
import { execSync } from "child_process";

export const dynamic = "force-dynamic";

const DB_CONTAINER = "gurotopia-mariadb";
const DB_PASS = "ukEhT<ZM3~&t)jI{";
const DB_NAME = "gurotopia";

export function GET() {
  try {
    const raw = execSync(
      `docker exec ${DB_CONTAINER} mariadb -uroot -p'${DB_PASS}' ${DB_NAME} -e "SELECT uid,growid,created_at FROM peer ORDER BY uid;" --batch 2>/dev/null`,
      { encoding: "utf-8", timeout: 5000 }
    ).trim();

    const lines = raw.split("\n").slice(1); // skip header
    const accounts = lines.map((line) => {
      const [uid, growid, created_at] = line.split("\t");
      return { uid, growid, created_at };
    }).filter((a) => a.uid && a.growid);

    return NextResponse.json({ accounts });
  } catch {
    return NextResponse.json({ accounts: [] });
  }
}
