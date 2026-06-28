import { NextResponse } from "next/server";
import { execSync } from "child_process";

export const dynamic = "force-dynamic";

const DB_CONTAINER = "gurotopia-mariadb";
const DB_PASS = "ukEhT<ZM3~&t)jI{";
const DB_NAME = "gurotopia";

export async function POST(request: Request) {
  const body = await request.json();
  const { growid, password } = body;

  if (!growid || growid.length < 3) {
    return NextResponse.json({ error: "GrowID too short (min 3 chars)" }, { status: 400 });
  }
  if (!password || password.length < 3) {
    return NextResponse.json({ error: "Password too short (min 3 chars)" }, { status: 400 });
  }
  if (growid.length > 18) {
    return NextResponse.json({ error: "GrowID too long (max 18 chars)" }, { status: 400 });
  }

  const safeGrowid = growid.replace(/'/g, "\\'");
  const safePassword = password.replace(/'/g, "\\'");

  try {
    // Check if growid already exists
    const existing = execSync(
      `docker exec ${DB_CONTAINER} mariadb -uroot -p'${DB_PASS}' ${DB_NAME} -e "SELECT uid FROM peer WHERE growid='${safeGrowid}';" --batch 2>/dev/null`,
      { encoding: "utf-8", timeout: 5000 }
    ).trim();

    if (existing.split("\n").length > 1) {
      return NextResponse.json({ error: "GrowID already taken" }, { status: 409 });
    }

    // Insert new account
    execSync(
      `docker exec ${DB_CONTAINER} mariadb -uroot -p'${DB_PASS}' ${DB_NAME} -e "INSERT INTO peer (growid, password) VALUES ('${safeGrowid}', '${safePassword}');" 2>/dev/null`,
      { encoding: "utf-8", timeout: 5000 }
    );

    // Get the new uid
    const raw = execSync(
      `docker exec ${DB_CONTAINER} mariadb -uroot -p'${DB_PASS}' ${DB_NAME} -e "SELECT uid, growid, created_at FROM peer WHERE growid='${safeGrowid}';" --batch 2>/dev/null`,
      { encoding: "utf-8", timeout: 5000 }
    ).trim();
    const lines = raw.split("\n").slice(1);
    const [uid, , created_at] = lines[0].split("\t");

    return NextResponse.json({
      success: true,
      account: { uid, growid: safeGrowid, created_at },
    });
  } catch (e) {
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
}
