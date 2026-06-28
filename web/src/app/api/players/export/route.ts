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

// GET - export all players as CSV
export async function GET() {
  try {
    const raw = dbQuery(
      `SELECT uid, growid, gems, level, xp, role, skin_color, hair_color, slot_size, banned, muted, created_at, last_daily, title, guild_id, pet_id, pet_name, pet_level, pet_xp FROM peer ORDER BY uid;`
    );

    const lines = raw.split("\n");

    // Build CSV
    const csvHeaders = [
      "uid", "growid", "gems", "level", "xp", "role",
      "skin_color", "hair_color", "slot_size", "banned", "muted",
      "created_at", "last_daily", "title", "guild_id",
      "pet_id", "pet_name", "pet_level", "pet_xp",
    ].join(",");

    const csvRows = lines
      .slice(1)
      .filter((l) => l.trim())
      .map((line) => {
        const cols = line.split("\t");
        // Escape CSV values that contain commas, quotes, or newlines
        return cols
          .map((c) => {
            if (!c) return "";
            const val = c.replace(/"/g, '""');
            return c.includes(",") || c.includes('"') || c.includes("\n") ? `"${val}"` : val;
          })
          .join(",");
      });

    const csv = [csvHeaders, ...csvRows].join("\n");

    return new NextResponse(csv, {
      headers: {
        "Content-Type": "text/csv",
        "Content-Disposition": `attachment; filename="players_export_${new Date().toISOString().split("T")[0]}.csv"`,
      },
    });
  } catch (e) {
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
}