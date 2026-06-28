import { NextResponse } from "next/server";
import { execSync } from "child_process";
import { readFileSync, writeFileSync } from "fs";

export const dynamic = "force-dynamic";

const CONFIG_PATH = "/root/gtps-host/gurotopia-runtime/config/server_data.php";

// GET - proxy config from GTPS server
export async function GET() {
  try {
    try {
      const raw = execSync(
        `curl -sk https://localhost:8444/growtopia/config`,
        { encoding: "utf-8", timeout: 5000 }
      ).trim();
      const data = JSON.parse(raw);
      return NextResponse.json(data);
    } catch {
      // Fallback: return empty config
      return NextResponse.json({
        config: {},
        message: "Server not reachable, showing empty config",
      });
    }
  } catch (e) {
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
}

// PATCH - update config values in server_data.php
export async function PATCH(request: Request) {
  try {
    const body = await request.json();
    const { updates } = body; // { key: value, ... }

    if (!updates || typeof updates !== "object") {
      return NextResponse.json({ error: "Missing updates object" }, { status: 400 });
    }

    // Read current config
    let content: string;
    try {
      content = readFileSync(CONFIG_PATH, "utf-8");
    } catch {
      return NextResponse.json(
        { error: "Cannot read server_data.php" },
        { status: 500 }
      );
    }

    // Simple find-and-replace for each config key
    // PHP configs look like: define('KEY', 'value');
    // or: $KEY = 'value';
    for (const [key, value] of Object.entries(updates)) {
      const safeKey = String(key).replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
      const safeVal = String(value);

      // Try define() pattern
      const definePattern = new RegExp(
        `(define\\s*\\(\\s*['\"]${safeKey}['\"]\\s*,\\s*['\"])[^'\"]*(['\"]\\s*\\))`,
        "g"
      );
      if (definePattern.test(content)) {
        content = content.replace(definePattern, `$1${safeVal}$2`);
        continue;
      }

      // Try $var = pattern
      const varPattern = new RegExp(
        `(\\$${safeKey}\\s*=\\s*['\"])[^'\"]*(['\"]\\s*;)`,
        "g"
      );
      if (varPattern.test(content)) {
        content = content.replace(varPattern, `$1${safeVal}$2`);
        continue;
      }
    }

    writeFileSync(CONFIG_PATH, content);

    return NextResponse.json({
      success: true,
      message: "Config updated. Restart server to apply changes.",
    });
  } catch (e) {
    return NextResponse.json({ success: false, error: String(e) }, { status: 500 });
  }
}