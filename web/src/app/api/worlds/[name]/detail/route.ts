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

interface ParsedDoor {
  label: string;
  destination: string;
  x: number;
  y: number;
}

interface ParsedObject {
  id: number;
  count: number;
  x: number;
  y: number;
}

function parseDoors(doorsRaw: string): ParsedDoor[] {
  if (!doorsRaw) return [];
  try {
    // Doors format might be JSON or custom CSV
    const trimmed = doorsRaw.trim();
    if (trimmed.startsWith("[") || trimmed.startsWith("{")) {
      return JSON.parse(trimmed);
    }
    // Try CSV: label,dest,x,y|label,dest,x,y
    return trimmed.split("|").filter(Boolean).map((d) => {
      const [label, destination, x, y] = d.split(",");
      return { label: label || "", destination: destination || "", x: parseInt(x) || 0, y: parseInt(y) || 0 };
    });
  } catch {
    return [];
  }
}

function parseObjects(objectsRaw: string): ParsedObject[] {
  if (!objectsRaw) return [];
  try {
    const trimmed = objectsRaw.trim();
    if (trimmed.startsWith("[") || trimmed.startsWith("{")) {
      return JSON.parse(trimmed);
    }
    // Try CSV: id,count,x,y|id,count,x,y
    return trimmed.split("|").filter(Boolean).map((o) => {
      const [id, count, x, y] = o.split(",");
      return { id: parseInt(id) || 0, count: parseInt(count) || 1, x: parseInt(x) || 0, y: parseInt(y) || 0 };
    });
  } catch {
    return [];
  }
}

function countBlocks(blocksRaw: string): number {
  if (!blocksRaw) return 0;
  try {
    const trimmed = blocksRaw.trim();
    if (trimmed.startsWith("[") || trimmed.startsWith("{")) {
      const arr = JSON.parse(trimmed);
      return Array.isArray(arr) ? arr.length : 0;
    }
    // CSV rows
    return trimmed.split("\n").filter(Boolean).length;
  } catch {
    return 0;
  }
}

function countDisplayItems(displaysRaw: string): number {
  if (!displaysRaw) return 0;
  try {
    const trimmed = displaysRaw.trim();
    if (trimmed.startsWith("[") || trimmed.startsWith("{")) {
      const arr = JSON.parse(trimmed);
      return Array.isArray(arr) ? arr.length : 0;
    }
    return trimmed.split("|").filter(Boolean).length;
  } catch {
    return 0;
  }
}

// GET - fetch world detail with block/object/door data
export async function GET(
  _request: Request,
  { params }: { params: Promise<{ name: string }> }
) {
  const { name } = await params;
  const safeName = decodeURIComponent(name).replace(/'/g, "\\\\'");

  try {
    const raw = dbQuery(
      `SELECT name, owner, is_public, lock_state, minimum_entry_level, blocks, objects, doors, displays, updated_at FROM world WHERE name='${safeName}' LIMIT 1;`
    );

    const lines = raw.split("\n").slice(1);
    if (lines.length === 0 || !lines[0]) {
      return NextResponse.json({ error: "World not found" }, { status: 404 });
    }

    const [dbName, owner, is_public, lock_state, minLevel, blocks, objects, doors, displays, updated_at] =
      lines[0].split("\t");

    const blockCount = countBlocks(blocks);
    const objectCount = parseObjects(objects).length;
    const doorCount = parseDoors(doors).length;
    const displayCount = countDisplayItems(displays);

    // Get owner info
    let ownerInfo: Record<string, unknown> | null = null;
    const ownerUid = parseInt(owner) || 0;
    if (ownerUid > 0) {
      const oRaw = dbQuery(
        `SELECT uid, growid FROM peer WHERE uid=${ownerUid} LIMIT 1;`
      );
      const oLines = oRaw.split("\n").slice(1);
      if (oLines.length > 0 && oLines[0]) {
        const [oUid, oGrowid] = oLines[0].split("\t");
        ownerInfo = { uid: parseInt(oUid) || 0, growid: oGrowid || "Unknown" };
      }
    }

    return NextResponse.json({
      world: {
        name: dbName,
        owner: ownerUid,
        owner_info: ownerInfo,
        is_public: parseInt(is_public) || 0,
        lock_state: parseInt(lock_state) || 0,
        minimum_entry_level: parseInt(minLevel) || 0,
        block_count: blockCount,
        object_count: objectCount,
        door_count: doorCount,
        display_count: displayCount,
        doors: parseDoors(doors),
        objects: parseObjects(objects),
        updated_at: updated_at || "",
        blocks_size: blocks ? blocks.length : 0,
        objects_size: objects ? objects.length : 0,
        doors_size: doors ? doors.length : 0,
      },
    });
  } catch (e) {
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
}