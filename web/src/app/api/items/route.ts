import { NextResponse } from "next/server";
import { execSync } from "child_process";

export const dynamic = "force-dynamic";

interface GTPSItem {
  id: number;
  name: string;
  type: string;
  rarity: string;
  category: string;
  properties: Record<string, unknown>;
}

// GET - proxy items from GTPS server
export async function GET(request: Request) {
  try {
    const { searchParams } = new URL(request.url);
    const page = searchParams.get("page") || "1";
    const search = searchParams.get("search") || "";

    // Fetch from server
    const raw = execSync(
      `curl -sk https://localhost:8444/growtopia/items?page=${page}`,
      { encoding: "utf-8", timeout: 15000 }
    ).trim();

    let data: { items?: GTPSItem[]; total?: number; pages?: number; page?: number } = {};
    try {
      data = JSON.parse(raw);
    } catch {
      return NextResponse.json({ error: "Failed to parse items response" }, { status: 502 });
    }

    let items = data.items || [];

    // Client-side search filter
    if (search) {
      const q = search.toLowerCase();
      items = items.filter((item) => item.name?.toLowerCase().includes(q));
    }

    return NextResponse.json({
      items,
      page: data.page || parseInt(page),
      totalPages: data.pages || 0,
      total: data.total || items.length,
    });
  } catch (e) {
    return NextResponse.json({ error: String(e), items: [] }, { status: 500 });
  }
}