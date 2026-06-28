import { NextResponse } from "next/server";
import { execSync } from "child_process";

export const dynamic = "force-dynamic";

const DB_CONTAINER = "gurotopia-mariadb";
const DB_PASS = "ukEhT<ZM3~&t)jI{";
const DB_NAME = "gurotopia";

function dbQuery(query: string): string {
  try {
    return execSync(
      `docker exec ${DB_CONTAINER} mariadb -uroot -p'${DB_PASS}' ${DB_NAME} -e "${query}" --batch 2>/dev/null`,
      { encoding: "utf-8", timeout: 5000 }
    ).trim();
  } catch {
    return "";
  }
}

export function GET() {
  try {
    // Total gems sum
    const totalGemsRaw = dbQuery("SELECT SUM(gems) as total_gems FROM peer;");
    const totalGemsMatch = totalGemsRaw.match(/(\d+)/);
    const totalGems = totalGemsMatch ? parseInt(totalGemsMatch[1]) : 0;

    // Player count
    const playerCountRaw = dbQuery("SELECT COUNT(*) as cnt FROM peer;");
    const cntMatch = playerCountRaw.match(/(\d+)/);
    const playerCount = cntMatch ? parseInt(cntMatch[1]) : 0;

    // Average gems
    const avgGems = playerCount > 0 ? Math.round(totalGems / playerCount) : 0;

    // Top 10 richest
    const richestRaw = dbQuery(
      "SELECT uid, growid, gems FROM peer ORDER BY gems DESC LIMIT 10;"
    );
    const richestLines = richestRaw
      .split("\n")
      .slice(1)
      .filter((l) => l.trim());
    const richest = richestLines.map((line) => {
      const [uid, growid, gems] = line.split("\t");
      return { uid: parseInt(uid) || 0, growid, gems: parseInt(gems) || 0 };
    });

    // Gems distribution: 0-100, 101-1000, 1001-10000, 10001-50000, 50001+
    const brackets = [
      { label: "0-100", min: 0, max: 100 },
      { label: "101-1K", min: 101, max: 1000 },
      { label: "1K-10K", min: 1001, max: 10000 },
      { label: "10K-50K", min: 10001, max: 50000 },
      { label: "50K+", min: 50001, max: 999999999 },
    ];
    const distribution = brackets.map((b) => {
      const r = dbQuery(
        `SELECT COUNT(*) as cnt FROM peer WHERE gems >= ${b.min} AND gems <= ${b.max};`
      );
      const m = r.match(/(\d+)/);
      return { ...b, count: m ? parseInt(m[1]) : 0 };
    });

    return NextResponse.json({
      totalGems,
      playerCount,
      avgGems,
      richest,
      distribution,
    });
  } catch (e) {
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
}