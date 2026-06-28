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

// GET - fetch single world metadata
export async function GET(
  _request: Request,
  { params }: { params: Promise<{ name: string }> }
) {
  const { name } = await params;
  const safeName = decodeURIComponent(name).replace(/'/g, "\\'");
  try {
    const raw = dbQuery(
      `SELECT name, owner, is_public, lock_state, minimum_entry_level, LENGTH(blocks) as blocks_size, LENGTH(objects) as objects_size, updated_at FROM world WHERE name='${safeName}' LIMIT 1;`
    );
    const lines = raw.split("\n").slice(1);
    if (lines.length === 0 || !lines[0]) {
      return NextResponse.json({ error: "World not found" }, { status: 404 });
    }
    const [
      dbName,
      owner,
      is_public,
      lock_state,
      minimum_entry_level,
      blocks_size,
      objects_size,
      updated_at,
    ] = lines[0].split("\t");
    return NextResponse.json({
      world: {
        name: dbName,
        owner,
        is_public: parseInt(is_public) || 0,
        lock_state,
        minimum_entry_level: parseInt(minimum_entry_level) || 0,
        blocks_size: parseInt(blocks_size) || 0,
        objects_size: parseInt(objects_size) || 0,
        updated_at,
      },
    });
  } catch (e) {
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
}

// DELETE - delete world by name
export async function DELETE(
  _request: Request,
  { params }: { params: Promise<{ name: string }> }
) {
  const { name } = await params;
  const safeName = decodeURIComponent(name).replace(/'/g, "\\'");
  try {
    dbQuery(`DELETE FROM world WHERE name='${safeName}';`);
    return NextResponse.json({ success: true, message: `World '${safeName}' deleted` });
  } catch (e) {
    return NextResponse.json({ success: false, error: String(e) }, { status: 500 });
  }
}
