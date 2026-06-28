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

// GET - fetch audit logs with optional filters
// Query params: ?action=ban&admin=1&limit=50&offset=0
export function GET(request: Request) {
  try {
    const { searchParams } = new URL(request.url);
    const action = searchParams.get("action");
    const admin = searchParams.get("admin");
    const limit = parseInt(searchParams.get("limit") || "50") || 50;
    const offset = parseInt(searchParams.get("offset") || "0") || 0;

    let whereClauses: string[] = [];
    if (action) {
      const safeAction = String(action).replace(/'/g, "\\'");
      whereClauses.push(`action='${safeAction}'`);
    }
    if (admin) {
      const safeAdmin = parseInt(admin) || 0;
      whereClauses.push(`admin_uid=${safeAdmin}`);
    }

    const whereSQL =
      whereClauses.length > 0 ? `WHERE ${whereClauses.join(" AND ")}` : "";

    const raw = dbQuery(
      `SELECT id, admin_uid, action, target_uid, detail, created_at FROM audit_log ${whereSQL} ORDER BY id DESC LIMIT ${limit} OFFSET ${offset};`
    );

    const lines = raw.split("\n").slice(1).filter((l) => l.trim());
    const logs = lines.map((line) => {
      const [id, admin_uid, actionCol, target_uid, detail, created_at] =
        line.split("\t");
      return {
        id: parseInt(id) || 0,
        admin_uid: parseInt(admin_uid) || 0,
        action: actionCol,
        target_uid: parseInt(target_uid) || 0,
        detail,
        created_at,
      };
    });

    // Get total count for pagination
    const countRaw = dbQuery(
      `SELECT COUNT(*) as cnt FROM audit_log ${whereSQL};`
    );
    const cntMatch = countRaw.match(/(\d+)/);
    const total = cntMatch ? parseInt(cntMatch[1]) : 0;

    return NextResponse.json({ logs, total, limit, offset });
  } catch (e) {
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
}

// DELETE - clear old audit logs
// Body: { olderThanDays: 90 } — delete logs older than N days
export async function DELETE(request: Request) {
  try {
    const body = await request.json().catch(() => ({}));
    const olderThanDays = parseInt(body.olderThanDays) || 90;

    dbQuery(
      `DELETE FROM audit_log WHERE created_at < DATE_SUB(NOW(), INTERVAL ${olderThanDays} DAY);`
    );

    return NextResponse.json({
      success: true,
      message: `Deleted audit logs older than ${olderThanDays} days`,
    });
  } catch (e) {
    return NextResponse.json(
      { success: false, error: String(e) },
      { status: 500 }
    );
  }
}