"use client";

import { useState, useCallback, useEffect } from "react";
import useSWR from "swr";
import { motion } from "framer-motion";
import { Server, Database, Users, Globe, RotateCw, RefreshCw, Cpu, HardDrive, Download } from "lucide-react";
import { Sidebar } from "@/components/layout/sidebar";
import { LivePlayers, BroadcastPanel, RoleManager, EconomyDashboard } from "@/components/sections/tier1";
import { AuditLog, ReportQueue, GuildManager } from "@/components/sections/tier2";
import { BackupManager, LiveConsole, WorldExplorer } from "@/components/sections/tier3";
import { PlayerGrowth, ActivityStats } from "@/components/sections/tier4";
import { PlayerDetail } from "@/components/sections/player-detail";
import { ItemsBrowser } from "@/components/sections/items-browser";
import { StoreManager } from "@/components/sections/store-manager";
import { ConfigPanel } from "@/components/sections/config-panel";
import { MailboxViewer } from "@/components/sections/mailbox-viewer";
import { WorldDetail } from "@/components/sections/world-detail";

const fetcher = (url: string) => fetch(url).then((r) => r.json());

interface StatusData {
  server: { running: boolean; status: string; ports: string };
  mariadb: { running: boolean; status: string; ports: string };
  uptime_seconds: number;
  player_count: number;
  server_data: Record<string, string>;
  timestamp: string;
}

interface Account {
  uid: string;
  growid: string;
  created_at: string;
}

interface Player {
  uid: number;
  growid: string;
  gems: number;
  level: number;
  xp: number;
  role: number;
  banned: number;
  muted: number;
  slot_size: number;
  created_at: string;
}

function formatUptime(s: number) {
  const h = Math.floor(s / 3600);
  const m = Math.floor((s % 3600) / 60);
  const sec = Math.floor(s % 60);
  if (h > 0) return `${h}h ${m}m`;
  if (m > 0) return `${m}m ${sec}s`;
  return `${sec}s`;
}

function StatCard({
  icon,
  label,
  value,
  sub,
  delay,
}: {
  icon: React.ReactNode;
  label: string;
  value: string;
  sub?: string;
  delay: number;
}) {
  return (
    <motion.div
      initial={{ opacity: 0, y: 16 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ delay, duration: 0.4, ease: "easeOut" }}
      className="rounded-lg border border-[#e7e5e4] dark:border-white/[0.08] bg-white dark:bg-white/[0.02] p-4 transition-colors"
    >
      <div className="w-7 h-7 rounded-md bg-stone-100 dark:bg-white/[0.05] flex items-center justify-center text-[#78716c] dark:text-white/40 mb-3 transition-colors">
        {icon}
      </div>
      <p className="text-[11px] text-[#a8a29e] dark:text-white/45 mb-0.5 transition-colors">
        {label}
      </p>
      <p className="text-[15px] font-semibold text-[#1c1917] dark:text-white/90 transition-colors">
        {value}
      </p>
      {sub && (
        <p className="text-[10px] text-[#a8a29e] dark:text-white/35 mt-0.5 transition-colors">
          {sub}
        </p>
      )}
    </motion.div>
  );
}

function LogLine({ line }: { line: string }) {
  let cls = "text-white/40";
  if (/tankIDName|login|enter_game/i.test(line)) cls = "text-emerald-400";
  else if (/quit|disconnect/i.test(line)) cls = "text-red-400/80";
  else if (/error|fail/i.test(line)) cls = "text-red-400";
  else if (/warn/i.test(line)) cls = "text-amber-400";
  else if (/broadcast/i.test(line)) cls = "text-cyan-400";
  return <div className={`text-[11px] font-mono ${cls}`}>{line}</div>;
}

// ─────────────────── Overview Section ───────────────────
function OverviewSection() {
  const { data } = useSWR<StatusData>("/api/status", fetcher, { refreshInterval: 5000 });
  const { data: metrics } = useSWR("/api/analytics/activity", fetcher, { refreshInterval: 15000 });

  const s = data;
  return (
    <div className="space-y-6">
      <div className="grid grid-cols-2 lg:grid-cols-4 gap-3">
        <StatCard
          icon={<Server size={13} />}
          label="Server Status"
          value={s?.server?.running ? "Online" : "Offline"}
          sub={s ? `Port: ${s.server?.ports || "17091"}` : "Loading..."}
          delay={0}
        />
        <StatCard
          icon={<Database size={13} />}
          label="MariaDB"
          value={s?.mariadb?.running ? "Healthy" : "Down"}
          sub={s ? `Port: ${s.mariadb?.ports || "3307"}` : "Loading..."}
          delay={0.1}
        />
        <StatCard
          icon={<Cpu size={13} />}
          label="Uptime"
          value={s ? formatUptime(s.uptime_seconds || 0) : "..."}
          sub="Server uptime"
          delay={0.2}
        />
        <StatCard
          icon={<Users size={13} />}
          label="Players"
          value={String(metrics?.onlineCount || s?.player_count || 0)}
          sub={`${metrics?.totalPlayers || 0} total registered`}
          delay={0.3}
        />
      </div>

      {/* Broadcast + Live Players */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <div className="rounded-lg border border-white/[0.08] bg-white/[0.01] p-4">
          <LivePlayers />
        </div>
        <div className="rounded-lg border border-white/[0.08] bg-white/[0.01] p-4">
          <BroadcastPanel />
        </div>
      </div>
    </div>
  );
}

// ─────────────────── Players Section ───────────────────
function PlayersSection({ onNavigate }: { onNavigate: (section: string, uid: number) => void }) {
  const { data, mutate } = useSWR("/api/players", fetcher, { refreshInterval: 10000 });
  const players: Player[] = data?.players || [];

  const handlePatch = async (uid: number, action: string, extra?: Record<string, any>) => {
    await fetch("/api/players", {
      method: "PATCH",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ uid, action, ...extra }),
    });
    mutate();
  };

  const handleExportCSV = () => {
    window.open("/api/players/export", "_blank");
  };

  return (
    <div className="space-y-4">
      <div className="flex items-center justify-between">
        <h2 className="text-sm font-medium text-white/70">All Players ({players.length})</h2>
        <div className="flex items-center gap-2">
          <button
            onClick={handleExportCSV}
            className="flex items-center gap-1 px-2 py-1 text-[10px] rounded-md bg-white/[0.08] text-white/50 hover:bg-white/[0.12] hover:text-white/70 transition-colors"
          >
            <Download size={11} />
            Export CSV
          </button>
          <button onClick={() => mutate()} className="p-1.5 text-white/30 hover:text-white/60 transition-colors">
            <RefreshCw size={13} />
          </button>
        </div>
      </div>
      <div className="overflow-x-auto rounded-lg border border-white/[0.06]">
        <table className="w-full text-xs">
          <thead>
            <tr className="text-white/30 border-b border-white/[0.06] bg-white/[0.01]">
              <th className="text-left py-2 px-3 font-normal">UID</th>
              <th className="text-left py-2 px-3 font-normal">GrowID</th>
              <th className="text-right py-2 px-3 font-normal">Level</th>
              <th className="text-right py-2 px-3 font-normal">Gems</th>
              <th className="text-center py-2 px-3 font-normal">Role</th>
              <th className="text-center py-2 px-3 font-normal">Status</th>
              <th className="text-right py-2 px-3 font-normal">Actions</th>
            </tr>
          </thead>
          <tbody>
            {players.map((p, i) => (
              <motion.tr
                key={p.uid}
                initial={{ opacity: 0 }}
                animate={{ opacity: 1 }}
                transition={{ delay: i * 0.005 }}
                className="border-b border-white/[0.03] hover:bg-white/[0.02]"
              >
                <td className="py-1.5 px-3 text-white/30 font-mono">{p.uid}</td>
                <td className="py-1.5 px-3">
                  <button
                    onClick={() => onNavigate("players-detail", p.uid)}
                    className="text-white/70 hover:text-white/90 hover:underline transition-colors cursor-pointer"
                  >
                    {p.growid}
                  </button>
                </td>
                <td className="py-1.5 px-3 text-right text-white/50">Lv.{p.level}</td>
                <td className="py-1.5 px-3 text-right text-white/60 font-mono">{p.gems?.toLocaleString()}</td>
                <td className="py-1.5 px-3 text-center">
                  <span className={`text-[10px] px-1.5 py-0.5 rounded ${
                    p.role === 3 ? "bg-red-500/10 text-red-400" :
                    p.role === 2 ? "bg-violet-500/10 text-violet-400" :
                    p.role === 1 ? "bg-amber-500/10 text-amber-400" :
                    "bg-white/5 text-white/30"
                  }`}>
                    {["Player","Mod","Dev","Admin"][p.role] || "Player"}
                  </span>
                </td>
                <td className="py-1.5 px-3 text-center">
                  {p.banned ? <span className="text-[10px] text-red-400">BANNED</span> :
                   p.muted ? <span className="text-[10px] text-amber-400">MUTED</span> :
                   <span className="text-[10px] text-emerald-400/70">Active</span>}
                </td>
                <td className="py-1.5 px-3 text-right">
                  <div className="flex gap-0.5 justify-end">
                    {!p.banned && (
                      <button onClick={() => handlePatch(p.uid, "ban")} className="px-1.5 py-0.5 text-[10px] rounded bg-red-500/10 text-red-400/70 hover:bg-red-500/20 transition-colors">Ban</button>
                    )}
                    {p.banned === 1 && (
                      <button onClick={() => handlePatch(p.uid, "unban")} className="px-1.5 py-0.5 text-[10px] rounded bg-emerald-500/10 text-emerald-400/70 hover:bg-emerald-500/20 transition-colors">Unban</button>
                    )}
                    {!p.muted && (
                      <button onClick={() => handlePatch(p.uid, "mute")} className="px-1.5 py-0.5 text-[10px] rounded bg-amber-500/10 text-amber-400/70 hover:bg-amber-500/20 transition-colors">Mute</button>
                    )}
                    {p.muted === 1 && (
                      <button onClick={() => handlePatch(p.uid, "unmute")} className="px-1.5 py-0.5 text-[10px] rounded bg-emerald-500/10 text-emerald-400/70 hover:bg-emerald-500/20 transition-colors">Unmute</button>
                    )}
                  </div>
                </td>
              </motion.tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}

// ─────────────────── Moderation Section ───────────────────
function ModerationSection() {
  return (
    <div className="space-y-6">
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <div className="rounded-lg border border-white/[0.08] bg-white/[0.01] p-4">
          <AuditLog />
        </div>
        <div className="rounded-lg border border-white/[0.08] bg-white/[0.01] p-4">
          <ReportQueue />
        </div>
      </div>
      <div className="rounded-lg border border-white/[0.08] bg-white/[0.01] p-4">
        <RoleManager />
      </div>
    </div>
  );
}

// ─────────────────── Analytics Section ───────────────────
function AnalyticsSection() {
  return (
    <div className="space-y-6">
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <div className="rounded-lg border border-white/[0.08] bg-white/[0.01] p-4">
          <PlayerGrowth />
        </div>
        <div className="rounded-lg border border-white/[0.08] bg-white/[0.01] p-4">
          <ActivityStats />
        </div>
      </div>
    </div>
  );
}

// ─────────────────── Guilds Section ───────────────────
function GuildsSection() {
  return (
    <div className="rounded-lg border border-white/[0.08] bg-white/[0.01] p-4">
      <GuildManager />
    </div>
  );
}

// ─────────────────── Worlds Section ───────────────────
function WorldsSection({ onNavigate }: { onNavigate: (section: string, name: string) => void }) {
  return (
    <div className="rounded-lg border border-white/[0.08] bg-white/[0.01] p-4">
      <WorldExplorer onNavigate={(name) => onNavigate("world-detail", name)} />
    </div>
  );
}

// ─────────────────── Backups Section ───────────────────
function BackupsSection() {
  return (
    <div className="rounded-lg border border-white/[0.08] bg-white/[0.01] p-4">
      <BackupManager />
    </div>
  );
}

// ─────────────────── Console Section ───────────────────
function ConsoleSection() {
  return (
    <div className="rounded-lg border border-white/[0.08] bg-white/[0.01] p-4">
      <LiveConsole />
    </div>
  );
}

// ─────────────────── Economy Section ───────────────────
function EconomySection() {
  return (
    <div className="rounded-lg border border-white/[0.08] bg-white/[0.01] p-4 max-w-2xl">
      <EconomyDashboard />
    </div>
  );
}

// ─────────────────── Logs Section ───────────────────
function LogsSection() {
  const { data, mutate } = useSWR("/api/logs?n=100", fetcher, { refreshInterval: 5000 });
  const logs: string[] = data?.logs || [];

  return (
    <div className="space-y-3">
      <div className="flex items-center gap-2">
        <h2 className="text-sm font-medium text-white/70">Server Logs</h2>
        <button onClick={() => mutate()} className="p-1 text-white/30 hover:text-white/60 transition-colors">
          <RefreshCw size={11} />
        </button>
      </div>
      <div className="rounded-lg border border-white/[0.06] bg-black/20 p-3 max-h-[500px] overflow-y-auto font-mono">
        {logs.map((line, i) => (
          <LogLine key={i} line={line} />
        ))}
      </div>
    </div>
  );
}

// ─────────────────── Accounts Section ───────────────────
function AccountsSection() {
  const { data } = useSWR("/api/accounts", fetcher);
  const accounts: Account[] = data?.accounts || [];

  return (
    <div className="space-y-3">
      <h2 className="text-sm font-medium text-white/70">Accounts ({accounts.length})</h2>
      <div className="overflow-x-auto rounded-lg border border-white/[0.06]">
        <table className="w-full text-xs">
          <thead>
            <tr className="text-white/30 border-b border-white/[0.06] bg-white/[0.01]">
              <th className="text-left py-2 px-3 font-normal">UID</th>
              <th className="text-left py-2 px-3 font-normal">GrowID</th>
              <th className="text-left py-2 px-3 font-normal">Created</th>
            </tr>
          </thead>
          <tbody>
            {accounts.map((a, i) => (
              <tr key={a.uid} className="border-b border-white/[0.03] hover:bg-white/[0.02]">
                <td className="py-1.5 px-3 text-white/30 font-mono">{a.uid}</td>
                <td className="py-1.5 px-3 text-white/70">{a.growid}</td>
                <td className="py-1.5 px-3 text-white/40">{a.created_at}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}

// ─────────────────── Main Page ───────────────────
export default function Home() {
  const [section, setSection] = useState("overview");
  const [playerDetailUid, setPlayerDetailUid] = useState(0);
  const [worldDetailName, setWorldDetailName] = useState("");

  const handleNavigate = (id: string, data?: number | string) => {
    if (id === "players-detail") {
      setPlayerDetailUid(data as number);
    } else if (id === "world-detail") {
      setWorldDetailName(data as string);
    }
    setSection(id);
  };

  return (
    <div className="flex min-h-screen bg-stone-50 dark:bg-[#0d0b0a] text-[#1c1917] dark:text-white/90">
      <Sidebar activeSection={section} onNavigate={(id) => handleNavigate(id)} />
      <main className="flex-1 px-6 lg:px-10 pt-12 md:pt-16 pb-24 max-w-5xl">
        {section === "overview" && <OverviewSection />}
        {section === "players" && (
          <PlayersSection onNavigate={(s, uid) => handleNavigate(s, uid)} />
        )}
        {section === "players-detail" && playerDetailUid > 0 && (
          <PlayerDetail uid={playerDetailUid} onBack={() => setSection("players")} />
        )}
        {section === "items" && <ItemsBrowser />}
        {section === "store" && <StoreManager />}
        {section === "economy" && <EconomySection />}
        {section === "worlds" && (
          <WorldsSection onNavigate={(s, name) => handleNavigate(s, name)} />
        )}
        {section === "world-detail" && worldDetailName && (
          <WorldDetail worldName={worldDetailName} onBack={() => setSection("worlds")} />
        )}
        {section === "moderation" && <ModerationSection />}
        {section === "guilds" && <GuildsSection />}
        {section === "analytics" && <AnalyticsSection />}
        {section === "backups" && <BackupsSection />}
        {section === "config" && <ConfigPanel />}
        {section === "mailbox" && <MailboxViewer />}
        {section === "console" && <ConsoleSection />}
        {section === "logs" && <LogsSection />}
        {section === "accounts" && <AccountsSection />}
      </main>
    </div>
  );
}