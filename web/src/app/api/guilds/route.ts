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

// GET - fetch all guilds with member count and leader info
export function GET() {
  try {
    const raw = dbQuery(
      `SELECT g.id, g.name, g.leader, p.growid as leader_growid, g.created_at, g.motd, (SELECT COUNT(*) FROM peer WHERE guild_id = g.id) as member_count FROM guild g LEFT JOIN peer p ON g.leader = p.uid ORDER BY g.name;`
    );

    const lines = raw.split("\n").slice(1).filter((l) => l.trim());
    const guilds = lines.map((line) => {
      const [id, name, leader, leader_growid, created_at, motd, member_count] =
        line.split("\t");
      return {
        id: parseInt(id) || 0,
        name,
        leader: parseInt(leader) || 0,
        leader_growid: leader_growid || "",
        created_at,
        motd: motd || "",
        member_count: parseInt(member_count) || 0,
      };
    });

    return NextResponse.json({ guilds });
  } catch (e) {
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
}

// DELETE - remove a guild (and unlink members)
// Body: { id: number }
export async function DELETE(request: Request) {
  try {
    const body = await request.json().catch(() => ({}));
    const { id } = body;

    if (!id) {
      return NextResponse.json(
        { error: "Missing required field: id" },
        { status: 400 }
      );
    }

    const safeId = parseInt(id) || 0;

    // Unlink members from this guild
    dbQuery(`UPDATE peer SET guild_id = 0 WHERE guild_id = ${safeId};`);

    // Delete the guild
    dbQuery(`DELETE FROM guild WHERE id = ${safeId};`);

    return NextResponse.json({
      success: true,
      message: `Guild #${safeId} deleted, members unlinked`,
    });
  } catch (e) {
    return NextResponse.json(
      { success: false, error: String(e) },
      { status: 500 }
    );
  }
}