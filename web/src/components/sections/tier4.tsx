"use client";

import { useState } from "react";
import useSWR from "swr";
import { motion } from "framer-motion";
import {
  TrendingUp,
  Activity,
  BarChart3,
} from "lucide-react";
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ResponsiveContainer,
  BarChart,
  Bar,
  PieChart,
  Pie,
  Cell,
} from "recharts";
import { format } from "date-fns";

const fetcher = (url: string) => fetch(url).then((r) => r.json());

// ─────────────────── Player Growth Chart ───────────────────

export function PlayerGrowth() {
  const [days, setDays] = useState(30);
  const { data, error } = useSWR(`/api/analytics/growth?days=${days}`, fetcher);

  if (error) return <div className="text-red-400 text-sm">Failed to load growth data</div>;
  if (!data) return <div className="text-white/40 text-sm animate-pulse">Loading charts...</div>;

  const chartData = (data.growth || []).map((d: any) => ({
    ...d,
    date: d.date ? format(new Date(d.date), "MM/dd") : "",
  }));

  return (
    <div>
      <div className="flex items-center justify-between mb-3">
        <div className="flex items-center gap-2">
          <TrendingUp size={14} className="text-white/40" />
          <span className="text-xs text-white/50">Player Growth</span>
        </div>
        <select
          value={days}
          onChange={(e) => setDays(parseInt(e.target.value))}
          className="bg-white/[0.05] border border-white/[0.1] rounded text-white/50 text-[10px] px-1.5 py-0.5 focus:outline-none"
        >
          <option value={7}>7 days</option>
          <option value={14}>14 days</option>
          <option value={30}>30 days</option>
          <option value={90}>90 days</option>
        </select>
      </div>

      <div className="h-[180px] w-full">
        <ResponsiveContainer width="100%" height="100%">
          <LineChart data={chartData} margin={{ top: 5, right: 5, left: -20, bottom: 0 }}>
            <CartesianGrid strokeDasharray="3 3" stroke="rgba(255,255,255,0.04)" />
            <XAxis
              dataKey="date"
              tick={{ fontSize: 9, fill: "rgba(255,255,255,0.3)" }}
              axisLine={{ stroke: "rgba(255,255,255,0.06)" }}
              tickLine={false}
              interval="preserveStartEnd"
            />
            <YAxis
              tick={{ fontSize: 9, fill: "rgba(255,255,255,0.3)" }}
              axisLine={false}
              tickLine={false}
              allowDecimals={false}
            />
            <Tooltip
              contentStyle={{
                background: "#1c1917",
                border: "1px solid rgba(255,255,255,0.1)",
                borderRadius: "6px",
                fontSize: "11px",
                color: "rgba(255,255,255,0.8)",
              }}
            />
            <Line
              type="monotone"
              dataKey="cumulative"
              name="Total Players"
              stroke="#a8a29e"
              strokeWidth={1.5}
              dot={false}
            />
            <Line
              type="monotone"
              dataKey="count"
              name="New Registrations"
              stroke="#d6d3d1"
              strokeWidth={1.5}
              dot={false}
              strokeDasharray="4 4"
            />
          </LineChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}

// ─────────────────── Activity Heatmap & Stats ───────────────────

const COLORS = ["#78716c", "#a8a29e", "#d6d3d1", "#e7e5e4", "#fafaf9"];

export function ActivityStats() {
  const { data, error } = useSWR("/api/analytics/activity", fetcher);

  if (error) return <div className="text-red-400 text-sm">Failed</div>;
  if (!data) return <div className="text-white/40 text-sm animate-pulse">Loading...</div>;

  // Role distribution pie
  const roleData = (data.roleDistribution || []).map((d: any) => ({
    name: d.role,
    value: d.count,
  }));

  // Level distribution
  const levelData = (data.levelDistribution || []).map((d: any) => ({
    name: d.range,
    players: d.count,
  }));

  return (
    <div>
      <div className="flex items-center gap-2 mb-3">
        <Activity size={14} className="text-white/40" />
        <span className="text-xs text-white/50">Activity &amp; Distribution</span>
      </div>

      {/* Quick stats */}
      <div className="grid grid-cols-4 gap-2 mb-3">
        <div className="bg-white/[0.03] rounded-md p-2 text-center">
          <div className="text-[10px] text-white/30">Total</div>
          <div className="text-sm font-semibold text-white/80">{data.totalPlayers || 0}</div>
        </div>
        <div className="bg-white/[0.03] rounded-md p-2 text-center">
          <div className="text-[10px] text-white/30">24h New</div>
          <div className="text-sm font-semibold text-emerald-400/80">
            {data.registrations24h || 0}
          </div>
        </div>
        <div className="bg-white/[0.03] rounded-md p-2 text-center">
          <div className="text-[10px] text-white/30">7d New</div>
          <div className="text-sm font-semibold text-white/80">
            {data.registrations7d || 0}
          </div>
        </div>
        <div className="bg-white/[0.03] rounded-md p-2 text-center">
          <div className="text-[10px] text-white/30">Online</div>
          <div className="text-sm font-semibold text-emerald-400">
            {data.onlineCount || 0}
          </div>
        </div>
      </div>

      {/* Level distribution bar chart */}
      <div className="mb-3">
        <div className="text-[10px] text-white/30 mb-1.5">Level Distribution</div>
        <div className="h-[100px] w-full">
          <ResponsiveContainer width="100%" height="100%">
            <BarChart data={levelData} margin={{ top: 0, right: 5, left: -20, bottom: 0 }}>
              <XAxis
                dataKey="name"
                tick={{ fontSize: 9, fill: "rgba(255,255,255,0.3)" }}
                axisLine={false}
                tickLine={false}
              />
              <Tooltip
                contentStyle={{
                  background: "#1c1917",
                  border: "1px solid rgba(255,255,255,0.1)",
                  borderRadius: "6px",
                  fontSize: "11px",
                  color: "rgba(255,255,255,0.8)",
                }}
              />
              <Bar dataKey="players" fill="#78716c" radius={[2, 2, 0, 0]} />
            </BarChart>
          </ResponsiveContainer>
        </div>
      </div>

      {/* Role distribution + ban/mute stats */}
      <div className="grid grid-cols-2 gap-3">
        <div>
          <div className="text-[10px] text-white/30 mb-1.5">Role Distribution</div>
          <div className="h-[80px] w-full">
            <ResponsiveContainer width="100%" height="100%">
              <PieChart>
                <Pie
                  data={roleData}
                  dataKey="value"
                  nameKey="name"
                  cx="50%"
                  cy="50%"
                  outerRadius={35}
                  innerRadius={20}
                >
                  {roleData.map((_: any, index: number) => (
                    <Cell key={index} fill={COLORS[index % COLORS.length]} />
                  ))}
                </Pie>
                <Tooltip
                  contentStyle={{
                    background: "#1c1917",
                    border: "1px solid rgba(255,255,255,0.1)",
                    borderRadius: "6px",
                    fontSize: "11px",
                    color: "rgba(255,255,255,0.8)",
                  }}
                />
              </PieChart>
            </ResponsiveContainer>
          </div>
        </div>
        <div>
          <div className="text-[10px] text-white/30 mb-1.5">Moderation</div>
          <div className="bg-white/[0.03] rounded-md p-2 space-y-1.5">
            <div className="flex justify-between text-[11px]">
              <span className="text-red-400/70">Banned</span>
              <span className="text-white/70">{data.bannedCount || 0}</span>
            </div>
            <div className="flex justify-between text-[11px]">
              <span className="text-amber-400/70">Muted</span>
              <span className="text-white/70">{data.mutedCount || 0}</span>
            </div>
            <div className="flex justify-between text-[11px]">
              <span className="text-white/40">Avg Level</span>
              <span className="text-white/70">{data.avgLevel || 1}</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}