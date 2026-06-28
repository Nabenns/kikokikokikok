"use client";

import { useState } from "react";
import useSWR from "swr";
import { motion } from "framer-motion";
import {
  ScrollText,
  Flag,
  Building,
  CheckCircle,
  XCircle,
  Trash2,
  Search,
} from "lucide-react";
import { format } from "date-fns";

const fetcher = (url: string) => fetch(url).then((r) => r.json());

// ─────────────────── Audit Log ───────────────────

export function AuditLog() {
  const [action, setAction] = useState("");
  const { data, error } = useSWR(
    `/api/audit?limit=50${action ? `&action=${action}` : ""}`,
    fetcher,
    { refreshInterval: 15000 }
  );

  if (error) return <div className="text-red-400 text-sm">Failed</div>;

  const logs = data?.logs || [];

  return (
    <div>
      <div className="flex items-center justify-between mb-3">
        <div className="flex items-center gap-2">
          <ScrollText size={14} className="text-white/40" />
          <span className="text-xs text-white/50">Moderation Audit Log</span>
        </div>
        <select
          value={action}
          onChange={(e) => setAction(e.target.value)}
          className="bg-white/[0.05] border border-white/[0.1] rounded text-white/50 text-[10px] px-1.5 py-0.5 focus:outline-none"
        >
          <option value="">All Actions</option>
          <option value="ban">Ban</option>
          <option value="unban">Unban</option>
          <option value="mute">Mute</option>
          <option value="unmute">Unmute</option>
          <option value="kick">Kick</option>
          <option value="give">Give Item</option>
          <option value="setgems">Set Gems</option>
          <option value="setrole">Set Role</option>
          <option value="broadcast">Broadcast</option>
        </select>
      </div>

      {logs.length === 0 ? (
        <p className="text-sm text-white/30 py-4 text-center">No audit entries yet</p>
      ) : (
        <div className="space-y-0.5 max-h-[400px] overflow-y-auto">
          {logs.map((l: any, i: number) => {
            const actionColors: Record<string, string> = {
              ban: "text-red-400", unban: "text-emerald-400",
              mute: "text-amber-400", unmute: "text-emerald-400",
              kick: "text-red-400", give: "text-blue-400",
              setgems: "text-violet-400", setrole: "text-violet-400",
              broadcast: "text-cyan-400",
            };
            return (
              <motion.div
                key={l.id || i}
                initial={{ opacity: 0, x: -4 }}
                animate={{ opacity: 1, x: 0 }}
                transition={{ delay: i * 0.01 }}
                className="flex items-center gap-2 py-1 px-2 rounded hover:bg-white/[0.02] text-[11px]"
              >
                <span className={`font-mono w-14 shrink-0 ${actionColors[l.action] || "text-white/40"}`}>
                  [{l.action}]
                </span>
                <span className="text-white/50 shrink-0">
                  uid:{l.admin_uid} → {l.target_uid}
                </span>
                <span className="text-white/30 truncate">{l.detail}</span>
                <span className="text-white/20 ml-auto shrink-0">
                  {l.created_at ? format(new Date(l.created_at), "MM/dd HH:mm") : ""}
                </span>
              </motion.div>
            );
          })}
        </div>
      )}
    </div>
  );
}

// ─────────────────── Report Queue ───────────────────

export function ReportQueue() {
  const [filter, setFilter] = useState(0); // 0=pending, -1=all
  const { data, mutate } = useSWR(
    `/api/reports${filter >= 0 ? `?status=${filter}` : ""}`,
    fetcher,
    { refreshInterval: 15000 }
  );

  const handleResolve = async (id: number, status: number) => {
    await fetch("/api/reports", {
      method: "PATCH",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ id, status, handled_by: 0 }),
    });
    mutate();
  };

  const reports = data?.reports || [];

  return (
    <div>
      <div className="flex items-center justify-between mb-3">
        <div className="flex items-center gap-2">
          <Flag size={14} className="text-white/40" />
          <span className="text-xs text-white/50">Report Queue</span>
          {data?.total > 0 && (
            <span className="text-[10px] px-1.5 py-0.5 rounded bg-red-500/10 text-red-400">
              {data.total}
            </span>
          )}
        </div>
        <select
          value={filter}
          onChange={(e) => setFilter(parseInt(e.target.value))}
          className="bg-white/[0.05] border border-white/[0.1] rounded text-white/50 text-[10px] px-1.5 py-0.5 focus:outline-none"
        >
          <option value={0}>Pending</option>
          <option value={-1}>All</option>
          <option value={1}>Resolved</option>
          <option value={2}>Rejected</option>
        </select>
      </div>

      {reports.length === 0 ? (
        <p className="text-sm text-white/30 py-4 text-center">No reports</p>
      ) : (
        <div className="space-y-1 max-h-[400px] overflow-y-auto">
          {reports.map((r: any, i: number) => (
            <motion.div
              key={r.id}
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              transition={{ delay: i * 0.02 }}
              className="bg-white/[0.02] border border-white/[0.05] rounded-md p-2 text-xs"
            >
              <div className="flex items-start justify-between mb-1">
                <div>
                  <span className="text-white/60">{r.reporter_growid || r.reporter_uid}</span>
                  <span className="text-white/20 mx-1">→</span>
                  <span className="text-red-400/80">{r.reported_growid || r.reported_uid}</span>
                </div>
                <span className="text-white/20">
                  {r.created_at ? format(new Date(r.created_at), "MM/dd HH:mm") : ""}
                </span>
              </div>
              <div className="text-white/40 mb-2">{r.reason || "No reason"}</div>
              {r.status === 0 && (
                <div className="flex gap-1">
                  <button
                    onClick={() => handleResolve(r.id, 1)}
                    className="flex items-center gap-1 px-2 py-0.5 text-[10px] rounded bg-emerald-500/10 text-emerald-400 hover:bg-emerald-500/20 transition-colors"
                  >
                    <CheckCircle size={10} /> Resolve
                  </button>
                  <button
                    onClick={() => handleResolve(r.id, 2)}
                    className="flex items-center gap-1 px-2 py-0.5 text-[10px] rounded bg-red-500/10 text-red-400 hover:bg-red-500/20 transition-colors"
                  >
                    <XCircle size={10} /> Reject
                  </button>
                </div>
              )}
              {r.status !== 0 && (
                <span className={`text-[10px] ${r.status === 1 ? "text-emerald-400" : "text-red-400"}`}>
                  {r.status === 1 ? "Resolved" : "Rejected"}
                </span>
              )}
            </motion.div>
          ))}
        </div>
      )}
    </div>
  );
}

// ─────────────────── Guild Manager ───────────────────

export function GuildManager() {
  const { data, mutate } = useSWR("/api/guilds", fetcher);

  const handleDelete = async (id: number, name: string) => {
    if (!confirm(`Delete guild "${name}"? This cannot be undone.`)) return;
    await fetch("/api/guilds", {
      method: "DELETE",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ id }),
    });
    mutate();
  };

  const guilds = data?.guilds || [];

  return (
    <div>
      <div className="flex items-center gap-2 mb-3">
        <Building size={14} className="text-white/40" />
        <span className="text-xs text-white/50">Guilds ({guilds.length})</span>
      </div>

      {guilds.length === 0 ? (
        <p className="text-sm text-white/30 py-4 text-center">No guilds created yet</p>
      ) : (
        <div className="space-y-1 max-h-[400px] overflow-y-auto">
          {guilds.map((g: any, i: number) => (
            <motion.div
              key={g.id}
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              transition={{ delay: i * 0.02 }}
              className="flex items-center justify-between py-1.5 px-2 rounded hover:bg-white/[0.02] text-xs"
            >
              <div>
                <span className="text-white/70 font-medium">{g.name}</span>
                <span className="text-white/30 ml-2">
                  leader: {g.leader_growid || g.leader_uid}
                </span>
                <span className="text-white/20 ml-2">{g.member_count || 0} members</span>
              </div>
              <button
                onClick={() => handleDelete(g.id, g.name)}
                className="p-1 text-white/20 hover:text-red-400 transition-colors"
                title="Delete guild"
              >
                <Trash2 size={12} />
              </button>
            </motion.div>
          ))}
        </div>
      )}
    </div>
  );
}