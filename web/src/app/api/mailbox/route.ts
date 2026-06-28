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

// GET - query mailbox with optional recipient filter
export async function GET(request: Request) {
  try {
    const { searchParams } = new URL(request.url);
    const recipient = searchParams.get("recipient");

    let whereSQL = "";
    if (recipient) {
      const safeR = parseInt(recipient) || 0;
      whereSQL = `WHERE m.recipient_uid = ${safeR}`;
    }

    const raw = dbQuery(
      `SELECT m.id, m.recipient_uid, p.growid as recipient_name, m.sender_name, m.message, m.item_id, m.item_count, m.sent_at, m.is_read FROM mailbox m LEFT JOIN peer p ON m.recipient_uid = p.uid ${whereSQL} ORDER BY m.sent_at DESC LIMIT 500;`
    );

    const lines = raw.split("\n").slice(1).filter((l) => l.trim());
    const messages = lines.map((line) => {
      const [id, recipient_uid, recipient_name, sender_name, message, item_id, item_count, sent_at, is_read] =
        line.split("\t");
      return {
        id: parseInt(id) || 0,
        recipient_uid: parseInt(recipient_uid) || 0,
        recipient_name: recipient_name || `UID:${recipient_uid}`,
        sender_name: sender_name || "Unknown",
        message: message || "",
        item_id: parseInt(item_id) || 0,
        item_count: parseInt(item_count) || 0,
        sent_at: sent_at || "",
        is_read: parseInt(is_read) || 0,
      };
    });

    return NextResponse.json({ messages, count: messages.length });
  } catch (e) {
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
}

// DELETE - delete a mailbox entry by id
export async function DELETE(request: Request) {
  try {
    const body = await request.json();
    const { id } = body;

    if (!id) {
      return NextResponse.json({ error: "Missing id" }, { status: 400 });
    }

    const safeId = parseInt(id) || 0;
    dbQuery(`DELETE FROM mailbox WHERE id=${safeId};`);

    return NextResponse.json({ success: true, message: `Mail #${safeId} deleted` });
  } catch (e) {
    return NextResponse.json({ success: false, error: String(e) }, { status: 500 });
  }
}