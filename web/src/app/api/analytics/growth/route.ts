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

// GET - player registration growth over time
// Query params: ?days=30 (limit to last N days)
export function GET(request: Request) {
  try {
    const { searchParams } = new URL(request.url);
    const days = parseInt(searchParams.get("days") || "0") || 0;

    let dateFilter = "";
    if (days > 0) {
      dateFilter = `WHERE created_at >= DATE_SUB(NOW(), INTERVAL ${days} DAY)`;
    }

    const raw = dbQuery(
      `SELECT DATE(created_at) as date, COUNT(*) as count FROM peer ${dateFilter} GROUP BY DATE(created_at) ORDER BY date;`
    );

    const lines = raw.split("\n").slice(1).filter((l) => l.trim());
    const growth = lines.map((line) => {
      const [date, count] = line.split("\t");
      return { date, count: parseInt(count) || 0 };
    });

    // Cumulative total
    let cumulative = 0;
    const growthWithCumulative = growth.map((g) => {
      cumulative += g.count;
      return { ...g, cumulative };
    });

    // Total players
    const totalRaw = dbQuery("SELECT COUNT(*) as cnt FROM peer;");
    const totalMatch = totalRaw.match(/(\d+)/);
    const totalPlayers = totalMatch ? parseInt(totalMatch[1]) : 0;

    return NextResponse.json({
      growth: growthWithCumulative,
      totalPlayers,
      dateRange: days > 0 ? `last ${days} days` : "all time",
    });
  } catch (e) {
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
}