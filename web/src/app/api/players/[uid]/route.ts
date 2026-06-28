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

// GET - fetch single player full data including clothing and inventory
export async function GET(
  _request: Request,
  { params }: { params: Promise<{ uid: string }> }
) {
  const { uid } = await params;
  const safeUid = parseInt(uid) || 0;
  try {
    const raw = dbQuery(
      `SELECT uid, growid, gems, level, xp, role, banned, muted, skin_color, hair_color, slot_size, clothing, inventory, created_at FROM peer WHERE uid=${safeUid} LIMIT 1;`
    );
    const lines = raw.split("\n").slice(1);
    if (lines.length === 0 || !lines[0]) {
      return NextResponse.json({ error: "Player not found" }, { status: 404 });
    }
    const [
      dbUid,
      growid,
      gems,
      level,
      xp,
      role,
      banned,
      muted,
      skin_color,
      hair_color,
      slot_size,
      clothing,
      inventory,
      created_at,
    ] = lines[0].split("\t");
    return NextResponse.json({
      player: {
        uid: parseInt(dbUid) || 0,
        growid,
        gems: parseInt(gems) || 0,
        level: parseInt(level) || 0,
        xp: parseInt(xp) || 0,
        role,
        banned: parseInt(banned) || 0,
        muted: parseInt(muted) || 0,
        skin_color: parseInt(skin_color) || 0,
        hair_color: parseInt(hair_color) || 0,
        slot_size: parseInt(slot_size) || 0,
        clothing,
        inventory,
        created_at,
      },
    });
  } catch (e) {
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
}

// DELETE - delete player by uid
export async function DELETE(
  _request: Request,
  { params }: { params: Promise<{ uid: string }> }
) {
  const { uid } = await params;
  const safeUid = parseInt(uid) || 0;
  try {
    dbQuery(`DELETE FROM peer WHERE uid=${safeUid};`);
    return NextResponse.json({ success: true, message: `Player uid=${safeUid} deleted` });
  } catch (e) {
    return NextResponse.json({ success: false, error: String(e) }, { status: 500 });
  }
}

// PATCH - update player fields
export async function PATCH(
  request: Request,
  { params }: { params: Promise<{ uid: string }> }
) {
  const { uid } = await params;
  const safeUid = parseInt(uid) || 0;
  const body = await request.json();

  const numericFields = ["gems", "level", "xp", "banned", "muted", "slot_size"];
  const stringFields = ["role"];

  const updates: string[] = [];

  for (const field of numericFields) {
    if (body[field] !== undefined) {
      const val = parseInt(body[field]) || 0;
      updates.push(`${field}=${val}`);
    }
  }

  for (const field of stringFields) {
    if (body[field] !== undefined) {
      const val = String(body[field]).replace(/'/g, "\\'");
      updates.push(`${field}='${val}'`);
    }
  }

  if (updates.length === 0) {
    return NextResponse.json(
      { error: "No valid fields to update" },
      { status: 400 }
    );
  }

  try {
    for (const update of updates) {
      dbQuery(`UPDATE peer SET ${update} WHERE uid=${safeUid};`);
    }
    return NextResponse.json({
      success: true,
      message: `Player uid=${safeUid} updated`,
      fields: updates.map((u) => u.split("=")[0]),
    });
  } catch (e) {
    return NextResponse.json({ success: false, error: String(e) }, { status: 500 });
  }
}
