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

// GET - activity analytics
// Since no session logs, approximate from peer data: level distribution, role distribution,
// registration recency, and try to get online count from server endpoint
export async function GET() {
  try {
    // Total players
    const totalRaw = dbQuery("SELECT COUNT(*) as cnt FROM peer;");
    const totalMatch = totalRaw.match(/(\d+)/);
    const totalPlayers = totalMatch ? parseInt(totalMatch[1]) : 0;

    // Level distribution (brackets)
    const levelBrackets = [
      { label: "1-10", min: 1, max: 10 },
      { label: "11-25", min: 11, max: 25 },
      { label: "26-50", min: 26, max: 50 },
      { label: "51-75", min: 51, max: 75 },
      { label: "76-100", min: 76, max: 100 },
      { label: "101+", min: 101, max: 999999 },
    ];
    const levelDistribution = levelBrackets.map((b) => {
      const r = dbQuery(
        `SELECT COUNT(*) as cnt FROM peer WHERE level >= ${b.min} AND level <= ${b.max};`
      );
      const m = r.match(/(\d+)/);
      return { ...b, count: m ? parseInt(m[1]) : 0 };
    });

    // Average level
    const avgLevelRaw = dbQuery("SELECT AVG(level) as avg_level FROM peer;");
    const avgLevelMatch = avgLevelRaw.match(/(\d+\.?\d*)/);
    const avgLevel = avgLevelMatch
      ? Math.round(parseFloat(avgLevelMatch[1]) * 10) / 10
      : 0;

    // Role distribution
    const rolesRaw = dbQuery(
      "SELECT role, COUNT(*) as cnt FROM peer GROUP BY role ORDER BY role;"
    );
    const rolesLines = rolesRaw.split("\n").slice(1).filter((l) => l.trim());
    const roleDistribution = rolesLines.map((line) => {
      const [role, cnt] = line.split("\t");
      const roleNum = parseInt(role) || 0;
      const roleNames: Record<number, string> = {
        0: "Player",
        1: "VIP",
        2: "Moderator",
        3: "Admin",
        4: "Developer",
        5: "Owner",
      };
      return {
        role: roleNum,
        label: roleNames[roleNum] || `Role ${roleNum}`,
        count: parseInt(cnt) || 0,
      };
    });

    // Registrations in last 7 days
    const recentRaw = dbQuery(
      "SELECT COUNT(*) as cnt FROM peer WHERE created_at >= DATE_SUB(NOW(), INTERVAL 7 DAY);"
    );
    const recentMatch = recentRaw.match(/(\d+)/);
    const registrationsLast7Days = recentMatch ? parseInt(recentMatch[1]) : 0;

    // Registrations in last 24 hours
    const last24Raw = dbQuery(
      "SELECT COUNT(*) as cnt FROM peer WHERE created_at >= DATE_SUB(NOW(), INTERVAL 1 DAY);"
    );
    const last24Match = last24Raw.match(/(\d+)/);
    const registrationsLast24h = last24Match ? parseInt(last24Match[1]) : 0;

    // Ban count
    const bannedRaw = dbQuery("SELECT COUNT(*) as cnt FROM peer WHERE banned=1;");
    const bannedMatch = bannedRaw.match(/(\d+)/);
    const bannedCount = bannedMatch ? parseInt(bannedMatch[1]) : 0;

    // Muted count
    const mutedRaw = dbQuery("SELECT COUNT(*) as cnt FROM peer WHERE muted=1;");
    const mutedMatch = mutedRaw.match(/(\d+)/);
    const mutedCount = mutedMatch ? parseInt(mutedMatch[1]) : 0;

    // Try to get online players from server endpoint
    let onlineCount = 0;
    let onlinePlayers: string[] = [];
    try {
      const onlineRaw = execSync("curl -s http://localhost:8444/online", {
        encoding: "utf-8",
        timeout: 3000,
      }).trim();
      if (onlineRaw) {
        try {
          const onlineData = JSON.parse(onlineRaw);
          if (Array.isArray(onlineData)) {
            onlinePlayers = onlineData;
            onlineCount = onlineData.length;
          } else if (onlineData.count !== undefined) {
            onlineCount = onlineData.count;
          } else if (onlineData.players) {
            onlinePlayers = onlineData.players;
            onlineCount = onlineData.players.length;
          }
        } catch {
          // If not JSON, try counting lines
          const lines = onlineRaw.split("\n").filter((l) => l.trim());
          onlineCount = lines.length;
          onlinePlayers = lines;
        }
      }
    } catch {
      // Server not reachable
    }

    return NextResponse.json({
      totalPlayers,
      onlineCount,
      onlinePlayers,
      avgLevel,
      levelDistribution,
      roleDistribution,
      registrationsLast7Days,
      registrationsLast24h,
      bannedCount,
      mutedCount,
    });
  } catch (e) {
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
}