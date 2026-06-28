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

// GET - fetch all worlds with search and filter support
// Query params: ?search=TEXT (search by name), ?owner=UID (filter by owner), ?public=1 (only public)
export function GET(request: Request) {
  try {
    const { searchParams } = new URL(request.url);
    const search = searchParams.get("search");
    const owner = searchParams.get("owner");
    const isPublic = searchParams.get("public");

    let whereClauses: string[] = [];
    if (search) {
      const safeSearch = String(search).replace(/'/g, "\\'");
      whereClauses.push(`w.name LIKE '%${safeSearch}%'`);
    }
    if (owner) {
      const safeOwner = parseInt(owner) || 0;
      whereClauses.push(`w.owner=${safeOwner}`);
    }
    if (isPublic === "1" || isPublic === "true") {
      whereClauses.push(`w.is_public=1`);
    }

    const whereSQL =
      whereClauses.length > 0 ? `WHERE ${whereClauses.join(" AND ")}` : "";

    // Join with peer to get owner growid, and count visitors (if any tracking exists)
    const raw = dbQuery(
      `SELECT w.name, w.owner, p.growid as owner_growid, w.is_public, w.lock_state, w.minimum_entry_level, w.updated_at FROM world w LEFT JOIN peer p ON w.owner = p.uid ${whereSQL} ORDER BY w.name;`
    );

    const lines = raw.split("\n").slice(1).filter((l) => l.trim());
    const worlds = lines
      .map((line) => {
        const [
          name,
          ownerCol,
          owner_growid,
          is_public,
          lock_state,
          minimum_entry_level,
          updated_at,
        ] = line.split("\t");
        return {
          name,
          owner: parseInt(ownerCol) || 0,
          owner_growid: owner_growid || "Unknown",
          is_public: parseInt(is_public) || 0,
          lock_state: parseInt(lock_state) || 0,
          minimum_entry_level: parseInt(minimum_entry_level) || 0,
          updated_at,
        };
      })
      .filter((w) => w.name);

    // Total count
    const countRaw = dbQuery(
      `SELECT COUNT(*) as cnt FROM world w ${whereSQL};`
    );
    const cntMatch = countRaw.match(/(\d+)/);
    const total = cntMatch ? parseInt(cntMatch[1]) : 0;

    return NextResponse.json({ worlds, total });
  } catch (e) {
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
}