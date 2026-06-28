import { NextResponse } from "next/server";
import { execSync } from "child_process";

export const dynamic = "force-dynamic";

const CONTAINER = "gtps-gurotopia";

export async function POST() {
  try {
    const result = execSync(`docker restart ${CONTAINER}`, {
      encoding: "utf-8",
      timeout: 30000,
    }).trim();
    return NextResponse.json({ success: true, result: result || "restarted" });
  } catch (e) {
    return NextResponse.json({ success: false, error: String(e) }, { status: 500 });
  }
}
