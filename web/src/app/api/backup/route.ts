import { NextResponse } from "next/server";
import { execSync } from "child_process";
import { readdirSync, statSync } from "fs";
import { join } from "path";

export const dynamic = "force-dynamic";

const BACKUP_DIR = "/root/gtps-host/backup";
const BACKUP_SCRIPT = "/root/gtps-host/gurotopia-runtime/backup.sh";

// GET - list all backups
export function GET() {
  try {
    // Ensure backup directory exists
    execSync(`mkdir -p ${BACKUP_DIR}`, { timeout: 3000 });

    const files = readdirSync(BACKUP_DIR);
    const backups = files
      .filter(
        (f) =>
          f.endsWith(".sql") ||
          f.endsWith(".sql.gz") ||
          f.endsWith(".tar.gz")
      )
      .map((f) => {
        const filePath = join(BACKUP_DIR, f);
        try {
          const stat = statSync(filePath);
          return {
            name: f,
            size: stat.size,
            sizeFormatted: formatSize(stat.size),
            created:
              stat.birthtime?.toISOString() || stat.mtime.toISOString(),
            modified: stat.mtime.toISOString(),
          };
        } catch {
          return {
            name: f,
            size: 0,
            sizeFormatted: "0 B",
            created: "",
            modified: "",
          };
        }
      })
      .sort(
        (a, b) =>
          new Date(b.created).getTime() - new Date(a.created).getTime()
      );

    return NextResponse.json({ backups });
  } catch (e) {
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
}

// POST - create a new database backup using the existing backup script
export async function POST() {
  try {
    // Run the existing backup script (handles special password chars properly)
    const result = execSync(`bash ${BACKUP_SCRIPT}`, {
      encoding: "utf-8",
      timeout: 60000,
    }).trim();

    return NextResponse.json({
      success: true,
      result,
      message: "Backup created via backup.sh",
    });
  } catch (e) {
    return NextResponse.json(
      { success: false, error: String(e) },
      { status: 500 }
    );
  }
}

function formatSize(bytes: number): string {
  if (bytes === 0) return "0 B";
  const units = ["B", "KB", "MB", "GB"];
  const i = Math.floor(Math.log(bytes) / Math.log(1024));
  return `${(bytes / Math.pow(1024, i)).toFixed(1)} ${units[i]}`;
}