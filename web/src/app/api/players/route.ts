import { NextResponse } from "next/server";
import { execSync } from "child_process";
import { writeFileSync, unlinkSync } from "fs";

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

// GET - fetch all players with full data, or filter ?online=true
export async function GET(request: Request) {
  try {
    const { searchParams } = new URL(request.url);
    const onlineOnly = searchParams.get("online") === "true";
    const search = searchParams.get("search");

    let onlineUids: number[] | null = null;

    if (onlineOnly) {
      // Query the game server for currently online IDs
      try {
        const onlineRaw = execSync("curl -s http://localhost:8444/online", {
          encoding: "utf-8",
          timeout: 3000,
        }).trim();
        if (onlineRaw) {
          try {
            const onlineData = JSON.parse(onlineRaw);
            if (Array.isArray(onlineData)) {
              // Array of strings or objects
              onlineUids = onlineData.map((o: unknown) => {
                if (typeof o === "object" && o !== null && "uid" in o) {
                  return parseInt((o as Record<string, unknown>).uid as string) || 0;
                }
                // assume it's a name/number string
                return parseInt(String(o)) || 0;
              }).filter((n: number) => n > 0);
            }
          } catch {
            // Not JSON, try parsing text
            const lines = onlineRaw.split("\n").filter((l) => l.trim());
            onlineUids = lines
              .map((l) => parseInt(l) || 0)
              .filter((n) => n > 0);
          }
        }
      } catch {
        // Server not reachable
        onlineUids = [];
      }
    }

    let whereSQL = "";
    if (onlineOnly && onlineUids !== null) {
      if (onlineUids.length === 0) {
        return NextResponse.json({ players: [], online: true });
      }
      const uidList = onlineUids.join(",");
      whereSQL = `WHERE uid IN (${uidList})`;
    } else if (search) {
      const safeSearch = String(search).replace(/'/g, "\\'");
      whereSQL = `WHERE growid LIKE '%${safeSearch}%'`;
    }

    const raw = dbQuery(
      `SELECT uid, growid, gems, level, xp, role, banned, muted, slot_size, created_at FROM peer ${whereSQL} ORDER BY uid;`
    );

    const lines = raw.split("\n").slice(1).filter((l) => l.trim());
    const players = lines
      .map((line) => {
        const [uid, growid, gems, level, xp, role, banned, muted, slot_size, created_at] =
          line.split("\t");
        return {
          uid: parseInt(uid) || 0,
          growid,
          gems: parseInt(gems) || 0,
          level: parseInt(level) || 0,
          xp: parseInt(xp) || 0,
          role,
          banned: parseInt(banned) || 0,
          muted: parseInt(muted) || 0,
          slot_size: parseInt(slot_size) || 0,
          created_at,
        };
      })
      .filter((p) => p.uid && p.growid);

    return NextResponse.json({ players, online: onlineOnly });
  } catch (e) {
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
}

// PATCH - batch player actions: role update, gems, ban/unban, mute/unmute, kick
// Body: { uid: number, action: "setrole"|"setgems"|"ban"|"unban"|"mute"|"unmute"|"kick", value?: string|number }
export async function PATCH(request: Request) {
  try {
    const body = await request.json();
    const { uid, action, value } = body;

    if ((!uid && action !== "broadcast" && action !== "servercmd") || !action) {
      return NextResponse.json(
        { error: "Missing required fields: uid, action" },
        { status: 400 }
      );
    }

    const safeUid = parseInt(uid) || 0;

    switch (action) {
      case "broadcast": {
        const msg = String(body.message || value || "");
        if (!msg) return NextResponse.json({ error: "Missing message" }, { status: 400 });
        try {
          const payload = JSON.stringify({ admin: "WebAdmin", message: msg });
          writeFileSync("/tmp/bcast.json", payload);
          execSync(
            "curl -sk -X POST https://localhost:8444/growtopia/broadcast -H 'Content-Type: application/json' -d @/tmp/bcast.json",
            { encoding: "utf-8", timeout: 5000 }
          );
          unlinkSync("/tmp/bcast.json");
          return NextResponse.json({ success: true, message: "Broadcast sent" });
        } catch (e) {
          return NextResponse.json({ error: String(e) }, { status: 500 });
        }
      }

      case "servercmd": {
        const cmd = String(body.command || value || "");
        if (!cmd) return NextResponse.json({ error: "Missing command" }, { status: 400 });
        try {
          const result = execSync(
            `docker exec gtps-gurotopia sh -c 'echo "${cmd.replace(/'/g, "'\\''")}"' 2>/dev/null`,
            { encoding: "utf-8", timeout: 5000 }
          ).trim();
          return NextResponse.json({ success: true, result });
        } catch (e) {
          return NextResponse.json({ error: String(e) }, { status: 500 });
        }
      }

      case "setrole": {
        const safeRole = String(value || "0").replace(/'/g, "\\'");
        dbQuery(`UPDATE peer SET role='${safeRole}' WHERE uid=${safeUid};`);
        // Also notify the game server
        try {
          execSync(
            `curl -s "http://localhost:8444/setrole?uid=${safeUid}&role=${encodeURIComponent(safeRole)}"`,
            { timeout: 3000 }
          );
        } catch { /* ignore server notify failure */ }
        return NextResponse.json({
          success: true,
          message: `Player uid=${safeUid} role set to '${safeRole}'`,
        });
      }

      case "setgems": {
        const safeGems = parseInt(String(value)) || 0;
        dbQuery(`UPDATE peer SET gems=${safeGems} WHERE uid=${safeUid};`);
        return NextResponse.json({
          success: true,
          message: `Player uid=${safeUid} gems set to ${safeGems}`,
        });
      }

      case "ban": {
        dbQuery(`UPDATE peer SET banned=1 WHERE uid=${safeUid};`);
        // Try to kick via server
        try {
          execSync(
            `curl -s "http://localhost:8444/kick?uid=${safeUid}"`,
            { timeout: 3000 }
          );
        } catch { /* ignore */ }
        return NextResponse.json({
          success: true,
          message: `Player uid=${safeUid} banned`,
        });
      }

      case "unban": {
        dbQuery(`UPDATE peer SET banned=0 WHERE uid=${safeUid};`);
        return NextResponse.json({
          success: true,
          message: `Player uid=${safeUid} unbanned`,
        });
      }

      case "mute": {
        dbQuery(`UPDATE peer SET muted=1 WHERE uid=${safeUid};`);
        try {
          execSync(
            `curl -s "http://localhost:8444/mute?uid=${safeUid}"`,
            { timeout: 3000 }
          );
        } catch { /* ignore */ }
        return NextResponse.json({
          success: true,
          message: `Player uid=${safeUid} muted`,
        });
      }

      case "unmute": {
        dbQuery(`UPDATE peer SET muted=0 WHERE uid=${safeUid};`);
        return NextResponse.json({
          success: true,
          message: `Player uid=${safeUid} unmuted`,
        });
      }

      case "kick": {
        try {
          const result = execSync(
            `curl -s "http://localhost:8444/kick?uid=${safeUid}"`,
            { encoding: "utf-8", timeout: 5000 }
          ).trim();
          return NextResponse.json({
            success: true,
            message: `Player uid=${safeUid} kicked`,
            serverResponse: result,
          });
        } catch (e) {
          return NextResponse.json(
            { success: false, error: `Kick failed: ${String(e)}` },
            { status: 500 }
          );
        }
      }

      default:
        return NextResponse.json(
          {
            error: `Unknown action: ${action}. Valid: broadcast, servercmd, setrole, setgems, ban, unban, mute, unmute, kick`,
          },
          { status: 400 }
        );
    }
  } catch (e) {
    return NextResponse.json(
      { success: false, error: String(e) },
      { status: 500 }
    );
  }
}