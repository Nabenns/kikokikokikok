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

// GET - fetch single account by uid
export async function GET(
  _request: Request,
  { params }: { params: Promise<{ uid: string }> }
) {
  const { uid } = await params;
  const safeUid = parseInt(uid) || 0;
  const raw = dbQuery(`SELECT uid, growid, password, created_at FROM peer WHERE uid=${safeUid};`);
  const lines = raw.split("\n").slice(1);
  if (lines.length === 0 || !lines[0]) {
    return NextResponse.json({ error: "Account not found" }, { status: 404 });
  }
  const [dbUid, growid, password, created_at] = lines[0].split("\t");
  return NextResponse.json({ account: { uid: dbUid, growid, password, created_at } });
}

// DELETE - delete account by uid
export async function DELETE(
  _request: Request,
  { params }: { params: Promise<{ uid: string }> }
) {
  const { uid } = await params;
  const safeUid = parseInt(uid) || 0;
  try {
    dbQuery(`DELETE FROM peer WHERE uid=${safeUid};`);
    return NextResponse.json({ success: true, message: `Account uid=${safeUid} deleted` });
  } catch (e) {
    return NextResponse.json({ success: false, error: String(e) }, { status: 500 });
  }
}

// PATCH - update account (change password)
export async function PATCH(
  request: Request,
  { params }: { params: Promise<{ uid: string }> }
) {
  const { uid } = await params;
  const safeUid = parseInt(uid) || 0;
  const body = await request.json();
  const { password } = body;
  if (!password || password.length < 3) {
    return NextResponse.json({ error: "Password too short (min 3 chars)" }, { status: 400 });
  }
  try {
    dbQuery(`UPDATE peer SET password='${password.replace(/'/g, "\\'")}' WHERE uid=${safeUid};`);
    return NextResponse.json({ success: true, message: "Password updated" });
  } catch (e) {
    return NextResponse.json({ success: false, error: String(e) }, { status: 500 });
  }
}
