"use client";

import { useState, useEffect, useRef, useCallback } from "react";
import useSWR from "swr";
import { motion } from "framer-motion";
import {
  HardDrive,
  Download,
  Upload,
  Save,
  Terminal,
  Globe,
  Search,
  RefreshCw,
} from "lucide-react";
import { format, formatDistanceToNow } from "date-fns";

const fetcher = (url: string) => fetch(url).then((r) => r.json());

// ─────────────────── Backup Manager ───────────────────

export function BackupManager() {
  const { data, mutate } = useSWR("/api/backup", fetcher);
  const [creating, setCreating] = useState(false);
  const [restoring, setRestoring] = useState<string | null>(null);

  const handleCreate = async () => {
    setCreating(true);
    await fetch("/api/backup", { method: "POST" });
    setCreating(false);
    mutate();
  };

  const handleRestore = async (filename: string) => {
    if (!confirm(`Restore backup "${filename}"? This will overwrite the current database.`)) return;
    setRestoring(filename);
    await fetch("/api/backup", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ restore: filename }),
    });
    setRestoring(null);
  };

  const backups = data?.backups || [];

  return (
    <div>
      <div className="flex items-center justify-between mb-3">
        <div className="flex items-center gap-2">
          <HardDrive size={14} className="text-white/40" />
          <span className="text-xs text-white/50">
            Backups ({backups.length})
          </span>
        </div>
        <button
          onClick={handleCreate}
          disabled={creating}
          className="flex items-center gap-1 px-2 py-1 text-[10px] rounded-md bg-white/[0.08] text-white/60 hover:bg-white/[0.12] hover:text-white/80 disabled:opacity-30 transition-all"
        >
          <Save size={10} />
          {creating ? "Creating..." : "Backup Now"}
        </button>
      </div>

      {backups.length === 0 ? (
        <p className="text-sm text-white/30 py-4 text-center">No backups yet</p>
      ) : (
        <div className="space-y-0.5 max-h-[400px] overflow-y-auto">
          {backups.map((b: any, i: number) => (
            <motion.div
              key={b.name}
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              transition={{ delay: i * 0.02 }}
              className="flex items-center justify-between py-1.5 px-2 rounded hover:bg-white/[0.02] text-xs"
            >
              <div className="flex items-center gap-2 min-w-0">
                <span className="text-white/60 truncate max-w-[180px]">{b.name}</span>
                <span className="text-white/20 shrink-0">{b.size}</span>
              </div>
              <div className="flex items-center gap-1 shrink-0">
                <span className="text-white/20 text-[10px] mr-1">
                  {b.modified ? formatDistanceToNow(new Date(b.modified), { addSuffix: true }) : ""}
                </span>
                <a
                  href={`/api/backup?download=${encodeURIComponent(b.name)}`}
                  className="p-1 text-white/30 hover:text-white/60 transition-colors"
                  title="Download"
                >
                  <Download size={11} />
                </a>
                <button
                  onClick={() => handleRestore(b.name)}
                  disabled={restoring === b.name}
                  className="p-1 text-white/30 hover:text-amber-400 transition-colors"
                  title="Restore"
                >
                  <Upload size={11} />
                </button>
              </div>
            </motion.div>
          ))}
        </div>
      )}
    </div>
  );
}

// ─────────────────── Live Console ───────────────────

export function LiveConsole() {
  const [lines, setLines] = useState<string[]>([]);
  const [cmd, setCmd] = useState("");
  const bottomRef = useRef<HTMLDivElement>(null);

  // Fetch logs
  useEffect(() => {
    const fetchLogs = async () => {
      try {
        const res = await fetch("/api/logs?n=30");
        const data = await res.json();
        setLines(data.logs || []);
      } catch {}
    };
    fetchLogs();
    const t = setInterval(fetchLogs, 5000);
    return () => clearInterval(t);
  }, []);

  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [lines]);

  const handleCmd = async () => {
    if (!cmd.trim()) return;
    if (cmd === "clear") { setLines([]); setCmd(""); return; }

    // Send via docker exec
    try {
      const res = await fetch("/api/players", {
        method: "PATCH",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ action: "servercmd", command: cmd }),
      });
      const data = await res.json();
      setLines((prev) => [
        ...prev,
        `> ${cmd}`,
        data.result || data.error || "Command sent",
      ]);
    } catch (e) {
      setLines((prev) => [...prev, `> ${cmd}`, `Error: Could not execute`]);
    }
    setCmd("");
  };

  const lineColor = (line: string) => {
    if (line.startsWith(">")) return "text-cyan-400";
    if (/error|fail|crash/i.test(line)) return "text-red-400";
    if (/login|connect|enter_game/i.test(line)) return "text-emerald-400";
    if (/warn/i.test(line)) return "text-amber-400";
    return "text-white/50";
  };

  return (
    <div>
      <div className="flex items-center gap-2 mb-3">
        <Terminal size={14} className="text-white/40" />
        <span className="text-xs text-white/50">Live Console</span>
      </div>

      <div className="bg-black/30 border border-white/[0.06] rounded-md p-2 h-[200px] overflow-y-auto font-mono text-[11px] leading-relaxed mb-2">
        {lines.length === 0 ? (
          <span className="text-white/20">Waiting for logs...</span>
        ) : (
          lines.map((line, i) => (
            <div key={i} className={lineColor(line)}>
              {line}
            </div>
          ))
        )}
        <div ref={bottomRef} />
      </div>

      <div className="flex gap-1">
        <input
          type="text"
          value={cmd}
          onChange={(e) => setCmd(e.target.value)}
          onKeyDown={(e) => { if (e.key === "Enter") handleCmd(); }}
          placeholder="Enter command or 'clear'..."
          className="flex-1 bg-white/[0.03] border border-white/[0.08] rounded-md px-2 py-1 text-[11px] text-white/70 placeholder:text-white/20 focus:outline-none focus:border-white/[0.15] font-mono"
        />
        <button
          onClick={handleCmd}
          className="px-2 py-1 text-[10px] rounded-md bg-white/[0.08] text-white/50 hover:bg-white/[0.12] transition-colors"
        >
          Run
        </button>
      </div>
    </div>
  );
}

// ─────────────────── World Explorer ───────────────────

export function WorldExplorer({ onNavigate }: { onNavigate?: (name: string) => void }) {
  const [search, setSearch] = useState("");
  const [debouncedSearch, setDebouncedSearch] = useState("");
  const { data, mutate } = useSWR(
    `/api/worlds?search=${debouncedSearch}&limit=50`,
    fetcher,
    { refreshInterval: 15000 }
  );

  useEffect(() => {
    const t = setTimeout(() => setDebouncedSearch(search), 300);
    return () => clearTimeout(t);
  }, [search]);

  const worlds = data?.worlds || [];

  return (
    <div>
      <div className="flex items-center gap-2 mb-3">
        <Globe size={14} className="text-white/40" />
        <span className="text-xs text-white/50">
          World Explorer ({data?.total || worlds.length})
        </span>
      </div>

      <div className="relative mb-2">
        <Search size={12} className="absolute left-2 top-1/2 -translate-y-1/2 text-white/20" />
        <input
          type="text"
          value={search}
          onChange={(e) => setSearch(e.target.value)}
          placeholder="Search worlds by name or owner..."
          className="w-full bg-white/[0.03] border border-white/[0.08] rounded-md pl-7 pr-2 py-1.5 text-[11px] text-white/70 placeholder:text-white/20 focus:outline-none focus:border-white/[0.15]"
        />
      </div>

      {worlds.length === 0 ? (
        <p className="text-sm text-white/30 py-4 text-center">
          {debouncedSearch ? "No worlds match your search" : "No worlds created yet"}
        </p>
      ) : (
        <div className="space-y-0.5 max-h-[350px] overflow-y-auto">
          {worlds.map((w: any, i: number) => (
            <motion.div
              key={w.name}
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              transition={{ delay: i * 0.01 }}
              className="flex items-center justify-between py-1.5 px-2 rounded hover:bg-white/[0.02] text-xs"
            >
              <div className="flex items-center gap-2 min-w-0">
                <span className="text-white/60 truncate max-w-[160px] font-mono">
                  {onNavigate ? (
                    <button
                      onClick={() => onNavigate(w.name)}
                      className="hover:text-white/90 hover:underline transition-colors cursor-pointer"
                    >
                      {w.name}
                    </button>
                  ) : (
                    w.name
                  )}
                </span>
                {w.is_public ? (
                  <span className="text-[9px] px-1 rounded bg-emerald-500/10 text-emerald-400">PUBLIC</span>
                ) : (
                  <span className="text-[9px] px-1 rounded bg-white/5 text-white/20">PRIVATE</span>
                )}
              </div>
              <div className="flex items-center gap-3 shrink-0">
                <span className="text-white/30">
                  owner: {w.owner_growid || w.owner || "none"}
                </span>
                <span className="text-white/20">
                  {w.blocks_size ? `${(w.blocks_size / 1024).toFixed(1)}KB` : "0KB"}
                </span>
                <span className="text-white/20">
                  {w.updated_at ? format(new Date(w.updated_at), "MM/dd") : ""}
                </span>
              </div>
            </motion.div>
          ))}
        </div>
      )}
    </div>
  );
}