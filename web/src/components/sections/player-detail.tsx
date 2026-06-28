"use client";

import useSWR from "swr";
import { motion } from "framer-motion";
import { ArrowLeft, User, Star, Coins, Zap, Shield, Package, Clock, PawPrint, ScrollText, Users } from "lucide-react";
import { format } from "date-fns";

const fetcher = (url: string) => fetch(url).then((r) => r.json());

function roleLabel(role: number) {
  switch (role) {
    case 3: return "Admin";
    case 2: return "Dev";
    case 1: return "Mod";
    default: return "Player";
  }
}

function roleBadgeColor(role: number) {
  switch (role) {
    case 3: return "bg-red-500/10 text-red-400";
    case 2: return "bg-violet-500/10 text-violet-400";
    case 1: return "bg-amber-500/10 text-amber-400";
    default: return "bg-white/5 text-white/50";
  }
}

interface PlayerDetailProps {
  uid: number;
  onBack: () => void;
}

export function PlayerDetail({ uid, onBack }: PlayerDetailProps) {
  const { data, error } = useSWR(`/api/players/${uid}/detail`, fetcher);

  if (error) return (
    <div className="text-red-400 text-sm">Failed to load player detail</div>
  );
  if (!data) return (
    <div className="text-white/40 text-sm animate-pulse">Loading player data...</div>
  );

  const { player, guild } = data;

  return (
    <motion.div
      initial={{ opacity: 0, y: 12 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.3 }}
      className="space-y-5"
    >
      {/* Header with back button */}
      <div className="flex items-center gap-3">
        <button
          onClick={onBack}
          className="p-1.5 rounded-md text-white/40 hover:text-white/70 hover:bg-white/[0.05] transition-colors"
        >
          <ArrowLeft size={16} />
        </button>
        <div>
          <div className="flex items-center gap-2">
            <h1 className="text-lg font-semibold text-white/90">{player.growid}</h1>
            <span className={`text-[10px] px-1.5 py-0.5 rounded font-medium ${roleBadgeColor(player.role)}`}>
              {roleLabel(player.role)}
            </span>
            {player.banned ? (
              <span className="text-[10px] px-1.5 py-0.5 rounded bg-red-500/10 text-red-400">BANNED</span>
            ) : null}
            {player.muted ? (
              <span className="text-[10px] px-1.5 py-0.5 rounded bg-amber-500/10 text-amber-400">MUTED</span>
            ) : null}
          </div>
          <p className="text-xs text-white/30 mt-0.5">UID: {player.uid}</p>
        </div>
      </div>

      {/* Stats Grid */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-3">
        <StatBox icon={<Star size={13} />} label="Level" value={`Lv.${player.level}`} />
        <StatBox icon={<Zap size={13} />} label="XP" value={player.xp?.toLocaleString()} />
        <StatBox icon={<Coins size={13} />} label="Gems" value={player.gems?.toLocaleString()} />
        <StatBox icon={<Clock size={13} />} label="Created" value={player.created_at ? format(new Date(player.created_at), "MMM dd, yyyy") : "N/A"} />
      </div>

      <div className="grid grid-cols-2 md:grid-cols-4 gap-3">
        <StatBox icon={<User size={13} />} label="Skin Color" value={`#${player.skin_color}`} />
        <StatBox icon={<User size={13} />} label="Hair Color" value={`#${player.hair_color}`} />
        <StatBox icon={<Package size={13} />} label="Slot Size" value={String(player.slot_size)} />
        <StatBox icon={<Clock size={13} />} label="Last Daily" value={player.last_daily ? format(new Date(player.last_daily), "MMM dd, HH:mm") : "Never"} />
      </div>

      {/* Title */}
      {player.title && (
        <div className="text-sm text-white/50">
          Title: <span className="text-white/80">{player.title}</span>
        </div>
      )}

      {/* Inventory */}
      <Section title="Inventory" sub={`${player.inventory?.length || 0} items`}>
        {player.inventory && player.inventory.length > 0 ? (
          <div className="grid grid-cols-3 md:grid-cols-6 gap-1.5">
            {player.inventory.map((item: { id: number; count: number }, i: number) => (
              <motion.div
                key={i}
                initial={{ opacity: 0, scale: 0.9 }}
                animate={{ opacity: 1, scale: 1 }}
                transition={{ delay: i * 0.01 }}
                className="bg-white/[0.03] border border-white/[0.06] rounded-md p-2 text-center"
              >
                <div className="text-[10px] text-white/30 font-mono">ID: {item.id}</div>
                <div className="text-xs text-white/70 font-medium">x{item.count}</div>
              </motion.div>
            ))}
          </div>
        ) : (
          <p className="text-sm text-white/20">No items in inventory</p>
        )}
      </Section>

      {/* Clothing Slots */}
      <Section title="Clothing Slots" sub="10 slots">
        <div className="flex flex-wrap gap-2">
          {player.clothing && player.clothing.map((val: number, i: number) => (
            <div
              key={i}
              className="bg-white/[0.03] border border-white/[0.06] rounded-md px-2 py-1 text-xs"
            >
              <span className="text-white/20">S{i+1}:</span>{" "}
              <span className="text-white/60 font-mono">{val}</span>
            </div>
          ))}
        </div>
      </Section>

      {/* Friends */}
      <Section title="Friends" sub={`${player.friends?.length || 0} friends`}>
        {player.friends && player.friends.length > 0 ? (
          <div className="flex flex-wrap gap-1.5">
            {player.friends.map((f: string, i: number) => (
              <span key={i} className="text-[10px] px-1.5 py-0.5 rounded bg-white/[0.04] text-white/50">{f}</span>
            ))}
          </div>
        ) : (
          <p className="text-sm text-white/20">No friends</p>
        )}
      </Section>

      {/* Pet Info */}
      {player.pet_id > 0 && (
        <Section title="Pet" sub={player.pet_name || `ID: ${player.pet_id}`}>
          <div className="grid grid-cols-3 gap-2 text-xs">
            <div className="bg-white/[0.03] rounded-md p-2"><span className="text-white/30">ID:</span> <span className="text-white/60">{player.pet_id}</span></div>
            <div className="bg-white/[0.03] rounded-md p-2"><span className="text-white/30">Level:</span> <span className="text-white/60">{player.pet_level}</span></div>
            <div className="bg-white/[0.03] rounded-md p-2"><span className="text-white/30">XP:</span> <span className="text-white/60">{player.pet_xp}</span></div>
          </div>
        </Section>
      )}

      {/* Quests */}
      <Section title="Quests" sub={`${player.quests?.length || 0} quests`}>
        {player.quests && player.quests.length > 0 ? (
          <div className="space-y-1">
            {player.quests.map((q: string, i: number) => (
              <div key={i} className="text-xs text-white/50 py-0.5 px-2 bg-white/[0.02] rounded">
                {q}
              </div>
            ))}
          </div>
        ) : (
          <p className="text-sm text-white/20">No quests</p>
        )}
      </Section>

      {/* Guild */}
      {guild && (
        <Section title="Guild">
          <div className="grid grid-cols-2 md:grid-cols-4 gap-2 text-xs">
            <div className="bg-white/[0.03] rounded-md p-2"><span className="text-white/30">Name:</span> <span className="text-white/60">{guild.name}</span></div>
            <div className="bg-white/[0.03] rounded-md p-2"><span className="text-white/30">Leader:</span> <span className="text-white/60">UID {guild.leader}</span></div>
            <div className="bg-white/[0.03] rounded-md p-2"><span className="text-white/30">Created:</span> <span className="text-white/60">{guild.created_at}</span></div>
            <div className="bg-white/[0.03] rounded-md p-2"><span className="text-white/30">MOTD:</span> <span className="text-white/60">{guild.motd || "N/A"}</span></div>
          </div>
        </Section>
      )}

      {/* Stats */}
      {player.stats && player.stats.length > 0 && (
        <Section title="Stats" sub={`${player.stats.length} fields`}>
          <div className="flex flex-wrap gap-1.5">
            {player.stats.map((s: string, i: number) => (
              <span key={i} className="text-[10px] px-1.5 py-0.5 rounded bg-white/[0.04] text-white/50 font-mono">{s}</span>
            ))}
          </div>
        </Section>
      )}
    </motion.div>
  );
}

function Section({ title, sub, children }: { title: string; sub?: string; children: React.ReactNode }) {
  return (
    <div className="rounded-lg border border-white/[0.06] bg-white/[0.01] p-4">
      <div className="flex items-center gap-2 mb-3">
        <h3 className="text-xs font-medium text-white/60">{title}</h3>
        {sub && <span className="text-[10px] text-white/25">{sub}</span>}
      </div>
      {children}
    </div>
  );
}

function StatBox({ icon, label, value }: { icon: React.ReactNode; label: string; value: string }) {
  return (
    <div className="rounded-md border border-white/[0.06] bg-white/[0.02] p-3">
      <div className="w-6 h-6 rounded bg-stone-100 dark:bg-white/[0.05] flex items-center justify-center text-white/40 mb-2">
        {icon}
      </div>
      <p className="text-[10px] text-white/30 mb-0.5">{label}</p>
      <p className="text-sm font-semibold text-white/80">{value}</p>
    </div>
  );
}