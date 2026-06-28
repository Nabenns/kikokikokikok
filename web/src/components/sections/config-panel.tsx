"use client";

import { useState } from "react";
import useSWR from "swr";
import { motion } from "framer-motion";
import { Settings, Save, RotateCw, RefreshCw, AlertTriangle } from "lucide-react";

const fetcher = (url: string) => fetch(url).then((r) => r.json());

export function ConfigPanel() {
  const { data, error, mutate } = useSWR("/api/config", fetcher);
  const [form, setForm] = useState<Record<string, string>>({});
  const [initialized, setInitialized] = useState(false);
  const [saving, setSaving] = useState(false);
  const [saved, setSaved] = useState(false);
  const [restartCountdown, setRestartCountdown] = useState(0);
  const [restarting, setRestarting] = useState(false);

  // Initialize form when data loads
  if (data?.config && !initialized) {
    setForm({ ...data.config });
    setInitialized(true);
  }

  const configFields = [
    { key: "SERVER_IP", label: "Server IP" },
    { key: "SERVER_PORT", label: "Port" },
    { key: "SERVER_TYPE", label: "Server Type" },
    { key: "SERVER_TYPE2", label: "Server Type 2" },
    { key: "MAINTENANCE_MESSAGE", label: "Maintenance Message" },
    { key: "LOGIN_URL", label: "Login URL" },
    { key: "META", label: "Meta" },
  ];

  const handleSave = async () => {
    setSaving(true);
    setSaved(false);
    try {
      const updates: Record<string, string> = {};
      for (const field of configFields) {
        if (form[field.key] !== undefined) {
          updates[field.key] = form[field.key];
        }
      }
      await fetch("/api/config", {
        method: "PATCH",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ updates }),
      });
      setSaved(true);
      setTimeout(() => setSaved(false), 3000);
    } catch {}
    setSaving(false);
  };

  const handleRestart = async () => {
    if (restartCountdown > 0) return;
    setRestartCountdown(5);
    const timer = setInterval(() => {
      setRestartCountdown((c) => {
        if (c <= 1) {
          clearInterval(timer);
          return 0;
        }
        return c - 1;
      });
    }, 1000);

    // Wait 5 seconds then restart
    setTimeout(async () => {
      setRestarting(true);
      try {
        await fetch("/api/restart", { method: "POST" });
      } catch {}
      setRestartCountdown(0);
      setRestarting(false);
    }, 5000);
  };

  return (
    <motion.div
      initial={{ opacity: 0, y: 12 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.3 }}
      className="space-y-4"
    >
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-2">
          <Settings size={14} className="text-white/40" />
          <h2 className="text-sm font-medium text-white/70">Server Configuration</h2>
        </div>
        <button onClick={() => mutate()} className="p-1.5 text-white/30 hover:text-white/60 transition-colors">
          <RefreshCw size={13} />
        </button>
      </div>

      {error && (
        <div className="text-red-400 text-xs">Failed to load config</div>
      )}

      {!data && !error && (
        <div className="text-white/30 text-sm animate-pulse">Loading config...</div>
      )}

      {data && (
        <div className="space-y-4">
          {/* Config Form */}
          <div className="rounded-lg border border-white/[0.06] bg-white/[0.01] p-4 space-y-3">
            {configFields.map((field) => {
              const value = form[field.key] ?? data?.config?.[field.key] ?? "";
              return (
                <div key={field.key}>
                  <label className="text-[10px] text-white/30 mb-1 block">{field.label}</label>
                  <input
                    type="text"
                    value={value}
                    onChange={(e) => setForm({ ...form, [field.key]: e.target.value })}
                    className="w-full bg-white/[0.03] border border-white/[0.08] rounded-md px-2 py-1.5 text-xs text-white/70 font-mono focus:outline-none focus:border-white/[0.15]"
                  />
                </div>
              );
            })}
          </div>

          {/* Any additional config keys */}
          {data?.config && (
            <div className="rounded-lg border border-white/[0.06] bg-white/[0.01] p-4">
              <div className="text-[10px] text-white/30 mb-2">All Config Values</div>
              <div className="bg-black/20 rounded-md p-2 text-[10px] font-mono text-white/40 max-h-[200px] overflow-y-auto">
                {JSON.stringify(data.config, null, 2)}
              </div>
            </div>
          )}

          {/* Actions */}
          <div className="flex items-center gap-3">
            <button
              onClick={handleSave}
              disabled={saving}
              className="flex items-center gap-1.5 px-3 py-1.5 text-xs rounded-md bg-white/[0.08] text-white/70 hover:bg-white/[0.12] hover:text-white/90 disabled:opacity-30 transition-all"
            >
              <Save size={12} />
              {saving ? "Saving..." : saved ? "Saved!" : "Save Config"}
            </button>

            <button
              onClick={handleRestart}
              disabled={restartCountdown > 0 || restarting}
              className="flex items-center gap-1.5 px-3 py-1.5 text-xs rounded-md bg-red-500/10 text-red-400/80 hover:bg-red-500/20 hover:text-red-400 disabled:opacity-30 transition-all"
            >
              <RotateCw size={12} className={restarting ? "animate-spin" : ""} />
              {restarting
                ? "Restarting..."
                : restartCountdown > 0
                ? `Restart in ${restartCountdown}s`
                : "Restart Server"}
            </button>
          </div>

          {restartCountdown > 0 && (
            <div className="flex items-center gap-2 text-xs text-amber-400">
              <AlertTriangle size={12} />
              Server will restart in {restartCountdown} seconds... All players will be disconnected.
            </div>
          )}
        </div>
      )}
    </motion.div>
  );
}