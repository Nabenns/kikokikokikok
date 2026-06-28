import { NextResponse } from "next/server";
import { execSync } from "child_process";
import { readFileSync, writeFileSync } from "fs";

export const dynamic = "force-dynamic";

const STORE_PATH = "/root/gtps-host/gurotopia-runtime/src/resources/store.txt";

interface StoreEntry {
  tab: string;
  btn: string;
  name: string;
  rttx: string;
  description: string;
  texture1: string;
  texture2: string;
  cost: number;
  items: string; // id:amount[,id:amount]
  raw: string;
}

function parseStoreLine(line: string): StoreEntry | null {
  // Format: tab|btn|name|rttx|description|texture1|texture2|cost|id:amount[,id:amount]
  const parts = line.split("|");
  if (parts.length < 8) return null;
  return {
    tab: parts[0] || "",
    btn: parts[1] || "",
    name: parts[2] || "",
    rttx: parts[3] || "",
    description: parts[4] || "",
    texture1: parts[5] || "",
    texture2: parts[6] || "",
    cost: parseInt(parts[7]) || 0,
    items: parts[8] || "",
    raw: line,
  };
}

function readStore(): StoreEntry[] {
  try {
    const content = readFileSync(STORE_PATH, "utf-8");
    return content
      .split("\n")
      .map((l) => l.trim())
      .filter((l) => l.length > 0 && !l.startsWith("#"))
      .map(parseStoreLine)
      .filter((e): e is StoreEntry => e !== null);
  } catch {
    return [];
  }
}

function writeStore(entries: StoreEntry[]) {
  const content = entries.map((e) => e.raw).join("\n") + "\n";
  writeFileSync(STORE_PATH, content);
}

function reloadStore() {
  try {
    execSync(
      `curl -sk -X POST https://localhost:8444/growtopia/store/reload`,
      { timeout: 5000 }
    );
  } catch {
    // ignore reload errors
  }
}

// GET - return all store entries
export async function GET() {
  try {
    // Try GTPS endpoint first
    try {
      const raw = execSync(
        `curl -sk https://localhost:8444/growtopia/store`,
        { encoding: "utf-8", timeout: 5000 }
      ).trim();
      const data = JSON.parse(raw);
      return NextResponse.json(data);
    } catch {
      // Fallback to reading file directly
      const entries = readStore();
      return NextResponse.json({ entries, count: entries.length });
    }
  } catch (e) {
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
}

// PATCH - add, remove, or update store entries
export async function PATCH(request: Request) {
  try {
    const body = await request.json();
    const { action, data } = body;

    if (!action) {
      return NextResponse.json({ error: "Missing action" }, { status: 400 });
    }

    const entries = readStore();

    switch (action) {
      case "add": {
        // data: { tab, btn, name, rttx, description, texture1, texture2, cost, items }
        const d = data || {};
        const line = [
          d.tab || "featured",
          d.btn || "",
          d.name || "Unknown",
          d.rttx || "",
          d.description || "",
          d.texture1 || "interface/store/icon.rttex",
          d.texture2 || "interface/store/icon.rttex",
          d.cost || 0,
          d.items || "",
        ].join("|");
        entries.push(parseStoreLine(line)!);
        writeStore(entries);
        reloadStore();
        return NextResponse.json({ success: true, message: "Store item added" });
      }

      case "remove": {
        // data: { index: number } - remove by index in the array
        const idx = parseInt(data?.index) ?? -1;
        if (idx < 0 || idx >= entries.length) {
          return NextResponse.json({ error: "Invalid index" }, { status: 400 });
        }
        const removed = entries[idx];
        entries.splice(idx, 1);
        writeStore(entries);
        reloadStore();
        return NextResponse.json({
          success: true,
          message: `Removed: ${removed.name}`,
        });
      }

      case "update": {
        // data: { index, ...fields }
        const idx = parseInt(data?.index) ?? -1;
        if (idx < 0 || idx >= entries.length) {
          return NextResponse.json({ error: "Invalid index" }, { status: 400 });
        }
        const existing = entries[idx];
        const fields = ["tab", "btn", "name", "rttx", "description", "texture1", "texture2"];
        const values = fields.map((f) => data?.[f] ?? existing[f as keyof StoreEntry] ?? "");
        const cost = data?.cost !== undefined ? data.cost : existing.cost;
        const items = data?.items !== undefined ? data.items : existing.items;
        const line = [...values, cost, items].join("|");
        entries[idx] = parseStoreLine(line)!;
        writeStore(entries);
        reloadStore();
        return NextResponse.json({ success: true, message: "Store item updated" });
      }

      default:
        return NextResponse.json(
          { error: `Unknown action: ${action}. Use add, remove, or update` },
          { status: 400 }
        );
    }
  } catch (e) {
    return NextResponse.json({ success: false, error: String(e) }, { status: 500 });
  }
}