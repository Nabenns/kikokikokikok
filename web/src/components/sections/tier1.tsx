"use client";

import { useState, useEffect, useCallback } from "react";
import useSWR from "swr";
import { motion, AnimatePresence } from "framer-motion";
import {
  Users,
  Shield,
  Sword,
  Gavel,
  MessageSquare,
  UserX,
  Gift,
  RefreshCw,
} from "lucide-react";

const fetcher = (url: string) => fetch(url).then((r) => r.json());

function roleLabel(role: number) {
  switch (role) {
    case 3: return "Admin";
    case 2: return "Dev";
    case 1: return "Mod";
    default: return "Player";
  }
}

function roleBadge(role: number) {
  const colors: Record<number, string> = {
    0: "bg-stone-100 dark:bg-white/5 text-stone-600 dark:text-white/50",
    1: "bg-amber-100 dark:bg-amber-500/10 text-amber-700 dark:text-amber-400",
    2: "bg-violet-100 dark:bg-violet-500/10 text-violet-700 dark:text-violet-400",
    3: "bg-red-100 dark:bg-red-500/10 text-red-700 dark:text-red-400",
  };
  return `text-[10px] px-1.5 py-0.5 rounded font-medium ${colors[role] || colors[0]}`;
}

export function LivePlayers() {
  const { data, error, mutate } = useSWR("/api/players?online=true", fetcher, {
    refreshInterval: 5000,
  });

  if (error) return <div className="text-red-400 text-sm">Failed to load players</div>;
  if (!data) return <div className="text-white/40 text-sm animate-pulse">Loading players...</div>;

  const players = data.players || [];

  return (
    <div>
      <div className="flex items-center justify-between mb-3">
        <div className="flex items-center gap-2">
          <div className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
          <span className="text-xs text-white/50">
            {players.length} online
          </span>
        </div>
        <button
          onClick={() => mutate()}
          className="p-1 text-white/30 hover:text-white/60 transition-colors"
        >
          <RefreshCw size={12} />
        </button>
      </div>

      {players.length === 0 ? (
        <p className="text-sm text-white/30">No players online</p>
      ) : (
        <div className="overflow-x-auto">
          <table className="w-full text-xs">
            <thead>
              <tr className="text-white/30 border-b border-white/[0.06]">
                <th className="text-left py-2 font-normal">Player</th>
                <th className="text-left py-2 font-normal">World</th>
                <th className="text-left py-2 font-normal">Role</th>
                <th className="text-right py-2 font-normal">Actions</th>
              </tr>
            </thead>
            <tbody>
              {players.map((p: any, i: number) => (
                <motion.tr
                  key={p.uid}
                  initial={{ opacity: 0, x: -8 }}
                  animate={{ opacity: 1, x: 0 }}
                  transition={{ delay: i * 0.03 }}
                  className="border-b border-white/[0.03] hover:bg-white/[0.02]"
                >
                  <td className="py-2">
                    <span className="text-white/80">{p.growid}</span>
                  </td>
                  <td className="py-2">
                    <span className="text-white/40">{p.world || "EXIT"}</span>
                  </td>
                  <td className="py-2">
                    <span className={roleBadge(p.role)}>{roleLabel(p.role)}</span>
                  </td>
                  <td className="py-2 text-right space-x-1">
                    <button
                      onClick={() => {
                        const name = prompt("Item ID:");
                        const count = prompt("Count:");
                        if (name && count) {
                          fetch("/api/players", {
                            method: "PATCH",
                            headers: { "Content-Type": "application/json" },
                            body: JSON.stringify({
                              uid: p.uid,
                              action: "give",
                              itemId: parseInt(name),
                              count: parseInt(count),
                            }),
                          });
                        }
                      }}
                      className="px-1.5 py-0.5 text-[10px] rounded bg-white/5 text-white/50 hover:bg-white/10 hover:text-white/80 transition-colors"
                      title="Give Item"
                    >
                      <Gift size={10} />
                    </button>
                    <button
                      onClick={() => {
                        if (confirm(`Kick ${p.growid}?`)) {
                          fetch("/api/players", {
                            method: "PATCH",
                            headers: { "Content-Type": "application/json" },
                            body: JSON.stringify({ uid: p.uid, action: "kick" }),
                          }).then(() => mutate());
                        }
                      }}
                      className="px-1.5 py-0.5 text-[10px] rounded bg-white/5 text-red-400/60 hover:bg-red-500/10 hover:text-red-400 transition-colors"
                      title="Kick"
                    >
                      <UserX size={10} />
                    </button>
                    <button
                      onClick={() => {
                        if (confirm(`Ban ${p.growid}?`)) {
                          fetch("/api/players", {
                            method: "PATCH",
                            headers: { "Content-Type": "application/json" },
                            body: JSON.stringify({ uid: p.uid, action: "ban" }),
                          }).then(() => mutate());
                        }
                      }}
                      className="px-1.5 py-0.5 text-[10px] rounded bg-white/5 text-red-400/60 hover:bg-red-500/10 hover:text-red-400 transition-colors"
                      title="Ban"
                    >
                      <Gavel size={10} />
                    </button>
                  </td>
                </motion.tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
}

export function BroadcastPanel() {
  const [msg, setMsg] = useState("");
  const [sent, setSent] = useState(false);
  const [sending, setSending] = useState(false);

  const handleSend = useCallback(async () => {
    if (!msg.trim()) return;
    setSending(true);
    try {
      await fetch("/api/players", {
        method: "PATCH",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ action: "broadcast", message: msg }),
      });
      setSent(true);
      setTimeout(() => setSent(false), 3000);
      setMsg("");
    } catch {}
    setSending(false);
  }, [msg]);

  return (
    <div>
      <div className="flex items-center gap-2 mb-3">
        <MessageSquare size={14} className="text-white/40" />
        <span className="text-xs text-white/50">Server Broadcast</span>
      </div>
      <textarea
        value={msg}
        onChange={(e) => setMsg(e.target.value)}
        onKeyDown={(e) => {
          if (e.key === "Enter" && !e.shiftKey) {
            e.preventDefault();
            handleSend();
          }
        }}
        placeholder="Enter broadcast message..."
        rows={2}
        className="w-full bg-white/[0.03] border border-white/[0.08] rounded-md px-3 py-2 text-xs text-white/80 placeholder:text-white/20 focus:outline-none focus:border-white/[0.15] resize-none transition-colors"
      />
      <div className="flex items-center justify-between mt-2">
        <AnimatePresence>
          {sent && (
            <motion.span
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              exit={{ opacity: 0 }}
              className="text-[10px] text-emerald-400"
            >
              Sent!
            </motion.span>
          )}
        </AnimatePresence>
        <button
          onClick={handleSend}
          disabled={sending || !msg.trim()}
          className="ml-auto px-3 py-1 text-[11px] rounded-md bg-white/[0.08] text-white/70 hover:bg-white/[0.12] hover:text-white/90 disabled:opacity-30 transition-all"
        >
          {sending ? "Sending..." : "Broadcast"}
        </button>
      </div>
    </div>
  );
}

export function RoleManager() {
  const { data, mutate } = useSWR("/api/players", fetcher);
  const [editing, setEditing] = useState<number | null>(null);

  const roleOptions = [
    { value: 0, label: "Player" },
    { value: 1, label: "Moderator" },
    { value: 2, label: "Developer" },
    { value: 3, label: "Admin" },
  ];

  const handleSetRole = async (uid: number, newRole: number) => {
    await fetch("/api/players", {
      method: "PATCH",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ uid, action: "setrole", role: newRole }),
    });
    setEditing(null);
    mutate();
  };

  if (!data) return <div className="text-white/40 text-sm animate-pulse">Loading...</div>;

  const players = (data.players || []).slice(0, 20);

  return (
    <div>
      <div className="flex items-center gap-2 mb-3">
        <Shield size={14} className="text-white/40" />
        <span className="text-xs text-white/50">Role Assignment</span>
      </div>
      <div className="overflow-x-auto">
        <table className="w-full text-xs">
          <tbody>
            {players.map((p: any, i: number) => (
              <motion.tr
                key={p.uid}
                initial={{ opacity: 0 }}
                animate={{ opacity: 1 }}
                transition={{ delay: i * 0.02 }}
                className="border-b border-white/[0.03]"
              >
                <td className="py-1.5">
                  <span className="text-white/70">{p.growid}</span>
                </td>
                <td className="py-1.5 text-right">
                  {editing === p.uid ? (
                    <select
                      value={p.role}
                      onChange={(e) => handleSetRole(p.uid, parseInt(e.target.value))}
                      className="bg-white/[0.05] border border-white/[0.1] rounded text-white/70 text-[10px] px-1.5 py-0.5 focus:outline-none"
                      autoFocus
                      onBlur={() => setEditing(null)}
                    >
                      {roleOptions.map((r) => (
                        <option key={r.value} value={r.value} className="bg-[#1c1917]">
                          {r.label}
                        </option>
                      ))}
                    </select>
                  ) : (
                    <button
                      onClick={() => setEditing(p.uid)}
                      className={roleBadge(p.role)}
                    >
                      {roleLabel(p.role)}
                    </button>
                  )}
                </td>
              </motion.tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}

export function EconomyDashboard() {
  const { data, error } = useSWR("/api/economy", fetcher, { refreshInterval: 30000 });

  if (error) return <div className="text-red-400 text-sm">Failed</div>;
  if (!data) return <div className="text-white/40 text-sm animate-pulse">Loading...</div>;

  return (
    <div>
      <div className="flex items-center gap-2 mb-3">
        <Users size={14} className="text-white/40" />
        <span className="text-xs text-white/50">Economy Overview</span>
      </div>

      <div className="grid grid-cols-3 gap-2 mb-3">
        <div className="bg-white/[0.03] rounded-md p-2 text-center">
          <div className="text-[10px] text-white/30">Total Gems</div>
          <div className="text-sm font-semibold text-white/80">
            {data.totalGems?.toLocaleString() || 0}
          </div>
        </div>
        <div className="bg-white/[0.03] rounded-md p-2 text-center">
          <div className="text-[10px] text-white/30">Players</div>
          <div className="text-sm font-semibold text-white/80">
            {data.playerCount || 0}
          </div>
        </div>
        <div className="bg-white/[0.03] rounded-md p-2 text-center">
          <div className="text-[10px] text-white/30">Avg Gems</div>
          <div className="text-sm font-semibold text-white/80">
            {data.avgGems?.toLocaleString() || 0}
          </div>
        </div>
      </div>

      {/* Distribution */}
      <div className="mb-3">
        <div className="text-[10px] text-white/30 mb-1.5">Gems Distribution</div>
        <div className="flex h-5 rounded overflow-hidden">
          {(data.distribution || []).map((d: any, i: number) => {
            const total = (data.distribution || []).reduce((s: number, x: any) => s + x.count, 0) || 1;
            const pct = ((d.count / total) * 100);
            const colors = [
              "bg-white/10", "bg-white/15", "bg-white/20", "bg-white/25", "bg-amber-500/30",
            ];
            return (
              <div
                key={i}
                className={`${colors[i]} h-full`}
                style={{ width: `${pct}%` }}
                title={`${d.label}: ${d.count}`}
              />
            );
          })}
        </div>
      </div>

      {/* Top 5 richest */}
      <div>
        <div className="text-[10px] text-white/30 mb-1.5">Richest Players</div>
        {(data.richest || []).slice(0, 5).map((p: any, i: number) => (
          <div key={p.uid} className="flex items-center justify-between py-0.5 text-xs">
            <span className="text-white/50">
              <span className="text-white/20 mr-1">#{i + 1}</span>
              {p.growid}
            </span>
            <span className="text-white/70 font-mono">{p.gems?.toLocaleString()}</span>
          </div>
        ))}
      </div>
    </div>
  );
}