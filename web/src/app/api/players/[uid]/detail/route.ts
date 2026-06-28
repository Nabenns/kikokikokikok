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

export async function generateStaticParams() {
  return [];
}

// GET - fetch full player detail with all fields
export async function GET(
  _request: Request,
  { params }: { params: Promise<{ uid: string }> }
) {
  const { uid } = await params;
  const safeUid = parseInt(uid) || 0;
  if (!safeUid) {
    return NextResponse.json({ error: "Invalid UID" }, { status: 400 });
  }

  try {
    const raw = dbQuery(
      `SELECT uid, growid, password, created_at, gems, level, xp, role, skin_color, hair_color, slot_size, clothing, inventory, banned, muted, last_daily, title, friends, guild_id, stats, pet_id, pet_name, pet_level, pet_xp, quests FROM peer WHERE uid=${safeUid} LIMIT 1;`
    );

    const lines = raw.split("\n").slice(1);
    if (lines.length === 0 || !lines[0]) {
      return NextResponse.json({ error: "Player not found" }, { status: 404 });
    }

    const cols = lines[0].split("\t");
    const [
      dbUid, growid, _password, created_at, gems, level, xp, role,
      skin_color, hair_color, slot_size, clothing, inventory,
      banned, muted, last_daily, title, friends, guild_id,
      stats, pet_id, pet_name, pet_level, pet_xp, quests,
    ] = cols;

    // Parse CSV fields
    const parseInventory = (csv: string) => {
      if (!csv) return [];
      return csv.split(",").map((entry) => {
        const [id, count] = entry.split(":");
        return { id: parseInt(id) || 0, count: parseInt(count) || 1 };
      }).filter((e) => e.id > 0);
    };

    const parseClothing = (csv: string) => {
      if (!csv) return [];
      return csv.split(",").map((v) => parseInt(v) || 0);
    };

    const parseList = (csv: string) => {
      if (!csv) return [];
      return csv.split(",").filter((v) => v.trim());
    };

    // Get guild name if in a guild
    let guild: Record<string, unknown> | null = null;
    const gid = parseInt(guild_id) || 0;
    if (gid > 0) {
      const gRaw = dbQuery(
        `SELECT id, name, leader, created_at, motd FROM guild WHERE id=${gid} LIMIT 1;`
      );
      const gLines = gRaw.split("\n").slice(1);
      if (gLines.length > 0 && gLines[0]) {
        const [gId, gName, gLeader, gCreated, gMotd] = gLines[0].split("\t");
        guild = {
          id: parseInt(gId) || 0,
          name: gName || "",
          leader: parseInt(gLeader) || 0,
          created_at: gCreated || "",
          motd: gMotd || "",
        };
      }
    }

    const player = {
      uid: parseInt(dbUid) || 0,
      growid: growid || "",
      created_at: created_at || "",
      gems: parseInt(gems) || 0,
      level: parseInt(level) || 0,
      xp: parseInt(xp) || 0,
      role: parseInt(role) || 0,
      skin_color: parseInt(skin_color) || 0,
      hair_color: parseInt(hair_color) || 0,
      slot_size: parseInt(slot_size) || 0,
      clothing: parseClothing(clothing),
      clothing_raw: clothing || "",
      inventory: parseInventory(inventory),
      inventory_raw: inventory || "",
      banned: parseInt(banned) || 0,
      muted: parseInt(muted) || 0,
      last_daily: last_daily || "",
      title: title || "",
      friends: parseList(friends),
      friends_raw: friends || "",
      guild_id: gid,
      stats: stats ? stats.split(",").map((s) => s.trim()) : [],
      stats_raw: stats || "",
      pet_id: parseInt(pet_id) || 0,
      pet_name: pet_name || "",
      pet_level: parseInt(pet_level) || 0,
      pet_xp: parseInt(pet_xp) || 0,
      quests: parseList(quests),
      quests_raw: quests || "",
    };

    return NextResponse.json({ player, guild });
  } catch (e) {
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
}