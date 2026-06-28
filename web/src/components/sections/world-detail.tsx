"use client";

import useSWR from "swr";
import { motion } from "framer-motion";
import { ArrowLeft, Globe, Lock, Unlock, Blocks, Box, DoorOpen, Monitor, Clock, User } from "lucide-react";
import { format, formatDistanceToNow } from "date-fns";

const fetcher = (url: string) => fetch(url).then((r) => r.json());

interface WorldDetailProps {
  worldName: string;
  onBack: () => void;
}

export function WorldDetail({ worldName, onBack }: WorldDetailProps) {
  const { data, error } = useSWR(`/api/worlds/${encodeURIComponent(worldName)}/detail`, fetcher);

  if (error) return (
    <div className="text-red-400 text-sm">Failed to load world detail</div>
  );
  if (!data) return (
    <div className="text-white/40 text-sm animate-pulse">Loading world data...</div>
  );

  const { world } = data;

  return (
    <motion.div
      initial={{ opacity: 0, y: 12 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.3 }}
      className="space-y-5"
    >
      {/* Header */}
      <div className="flex items-center gap-3">
        <button
          onClick={onBack}
          className="p-1.5 rounded-md text-white/40 hover:text-white/70 hover:bg-white/[0.05] transition-colors"
        >
          <ArrowLeft size={16} />
        </button>
        <div>
          <div className="flex items-center gap-2">
            <h1 className="text-lg font-semibold text-white/90 font-mono">{world.name}</h1>
            {world.is_public ? (
              <span className="text-[10px] px-1.5 py-0.5 rounded bg-emerald-500/10 text-emerald-400 flex items-center gap-1">
                <Unlock size={9} /> PUBLIC
              </span>
            ) : (
              <span className="text-[10px] px-1.5 py-0.5 rounded bg-red-500/10 text-red-400 flex items-center gap-1">
                <Lock size={9} /> PRIVATE
              </span>
            )}
          </div>
          <p className="text-xs text-white/30 mt-0.5">
            Owner: {world.owner_info?.growid || world.owner || "Unknown"} (UID: {world.owner})
          </p>
        </div>
      </div>

      {/* Stats Grid */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-3">
        <StatBox icon={<Blocks size={13} />} label="Blocks" value={world.block_count?.toLocaleString()} />
        <StatBox icon={<Box size={13} />} label="Objects" value={world.object_count?.toLocaleString()} />
        <StatBox icon={<DoorOpen size={13} />} label="Doors" value={world.door_count?.toLocaleString()} />
        <StatBox icon={<Monitor size={13} />} label="Displays" value={world.display_count?.toLocaleString()} />
      </div>

      {/* Properties */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-3">
        <StatBox icon={<Lock size={13} />} label="Lock State" value={world.lock_state === 0 ? "Unlocked" : `Level ${world.lock_state}`} />
        <StatBox icon={<User size={13} />} label="Min Level" value={String(world.minimum_entry_level)} />
        <StatBox icon={<Clock size={13} />} label="Updated" value={world.updated_at ? format(new Date(world.updated_at), "MMM dd, yyyy HH:mm") : "N/A"} />
        <div className="rounded-md border border-white/[0.06] bg-white/[0.02] p-3">
          <div className="w-6 h-6 rounded bg-white/[0.05] flex items-center justify-center text-white/40 mb-2">
            <Globe size={13} />
          </div>
          <p className="text-[10px] text-white/30 mb-0.5">Data Size</p>
          <p className="text-sm font-semibold text-white/80">
            {((world.blocks_size + world.objects_size + world.doors_size) / 1024).toFixed(1)} KB
          </p>
        </div>
      </div>

      {/* Doors List */}
      {world.doors && world.doors.length > 0 && (
        <Section title="Doors" sub={`${world.doors.length} doors`}>
          <div className="overflow-x-auto">
            <table className="w-full text-xs">
              <thead>
                <tr className="text-white/30 border-b border-white/[0.06]">
                  <th className="text-left py-1.5 px-2 font-normal">Label</th>
                  <th className="text-left py-1.5 px-2 font-normal">Destination</th>
                  <th className="text-right py-1.5 px-2 font-normal">X</th>
                  <th className="text-right py-1.5 px-2 font-normal">Y</th>
                </tr>
              </thead>
              <tbody>
                {world.doors.map((door: { label: string; destination: string; x: number; y: number }, i: number) => (
                  <tr key={i} className="border-b border-white/[0.03]">
                    <td className="py-1 px-2 text-white/60">{door.label || "-"}</td>
                    <td className="py-1 px-2 text-white/40 font-mono">{door.destination || "-"}</td>
                    <td className="py-1 px-2 text-right text-white/30">{door.x}</td>
                    <td className="py-1 px-2 text-right text-white/30">{door.y}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </Section>
      )}

      {/* Objects List (limited to first 50) */}
      {world.objects && world.objects.length > 0 && (
        <Section title="Objects" sub={`${world.objects.length} objects (showing first 50)`}>
          <div className="flex flex-wrap gap-1.5">
            {world.objects.slice(0, 50).map((obj: { id: number; count: number; x: number; y: number }, i: number) => (
              <div
                key={i}
                className="text-[10px] bg-white/[0.03] border border-white/[0.05] rounded px-1.5 py-0.5 font-mono text-white/50"
                title={`Pos: ${obj.x},${obj.y}`}
              >
                {obj.id}x{obj.count}
              </div>
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