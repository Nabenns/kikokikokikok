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

// GET - fetch reports with optional filters
// Query params: ?status=0 (0=pending, 1=resolved/rejected), ?reporter=UID, ?limit=50
export function GET(request: Request) {
  try {
    const { searchParams } = new URL(request.url);
    const status = searchParams.get("status");
    const reporter = searchParams.get("reporter");
    const limit = parseInt(searchParams.get("limit") || "50") || 50;
    const offset = parseInt(searchParams.get("offset") || "0") || 0;

    let whereClauses: string[] = [];
    if (status !== null && status !== "") {
      const safeStatus = parseInt(status) || 0;
      whereClauses.push(`r.status=${safeStatus}`);
    }
    if (reporter) {
      const safeReporter = parseInt(reporter) || 0;
      whereClauses.push(`r.reporter_uid=${safeReporter}`);
    }

    const whereSQL =
      whereClauses.length > 0 ? `WHERE ${whereClauses.join(" AND ")}` : "";

    // Join with peer to get growids for reporter and reported
    const raw = dbQuery(
      `SELECT r.id, r.reporter_uid, pr.growid as reporter_growid, r.reported_uid, pd.growid as reported_growid, r.reason, r.world_name, r.status, r.handled_by, r.created_at FROM reports r LEFT JOIN peer pr ON r.reporter_uid = pr.uid LEFT JOIN peer pd ON r.reported_uid = pd.uid ${whereSQL} ORDER BY r.id DESC LIMIT ${limit} OFFSET ${offset};`
    );

    const lines = raw.split("\n").slice(1).filter((l) => l.trim());
    const reports = lines.map((line) => {
      const [
        id,
        reporter_uid,
        reporter_growid,
        reported_uid,
        reported_growid,
        reason,
        world_name,
        statusCol,
        handled_by,
        created_at,
      ] = line.split("\t");
      return {
        id: parseInt(id) || 0,
        reporter_uid: parseInt(reporter_uid) || 0,
        reporter_growid: reporter_growid || "",
        reported_uid: parseInt(reported_uid) || 0,
        reported_growid: reported_growid || "",
        reason,
        world_name,
        status: parseInt(statusCol) || 0,
        handled_by: parseInt(handled_by) || 0,
        created_at,
      };
    });

    // Total count
    const countRaw = dbQuery(
      `SELECT COUNT(*) as cnt FROM reports r ${whereSQL};`
    );
    const cntMatch = countRaw.match(/(\d+)/);
    const total = cntMatch ? parseInt(cntMatch[1]) : 0;

    return NextResponse.json({ reports, total, limit, offset });
  } catch (e) {
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
}

// PATCH - resolve or reject a report
// Body: { id: number, status: 1|2 (1=resolved, 2=rejected), handled_by: number }
export async function PATCH(request: Request) {
  try {
    const body = await request.json();
    const { id, status, handled_by } = body;

    if (!id || status === undefined) {
      return NextResponse.json(
        { error: "Missing required fields: id, status" },
        { status: 400 }
      );
    }

    const safeId = parseInt(id) || 0;
    const safeStatus = parseInt(status) || 0;
    const updates: string[] = [`status=${safeStatus}`];

    if (handled_by !== undefined) {
      const safeHandledBy = parseInt(handled_by) || 0;
      updates.push(`handled_by=${safeHandledBy}`);
    }

    dbQuery(
      `UPDATE reports SET ${updates.join(", ")} WHERE id=${safeId};`
    );

    const statusText =
      safeStatus === 1 ? "resolved" : safeStatus === 2 ? "rejected" : "updated";

    return NextResponse.json({
      success: true,
      message: `Report #${safeId} ${statusText}`,
    });
  } catch (e) {
    return NextResponse.json(
      { success: false, error: String(e) },
      { status: 500 }
    );
  }
}