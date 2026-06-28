"use client";

import { useState } from "react";
import useSWR from "swr";
import { motion, AnimatePresence } from "framer-motion";
import { Store, Plus, Trash2, Edit3, X, Save, RefreshCw } from "lucide-react";
import { format } from "date-fns";

const fetcher = (url: string) => fetch(url).then((r) => r.json());

interface StoreEntry {
  tab: string;
  btn: string;
  name: string;
  rttx: string;
  description: string;
  texture1: string;
  texture2: string;
  cost: number;
  items: string;
  raw?: string;
  index?: number;
}

export function StoreManager() {
  const { data, error, mutate } = useSWR("/api/store", fetcher, { refreshInterval: 30000 });
  const [showModal, setShowModal] = useState(false);
  const [editingIndex, setEditingIndex] = useState<number | null>(null);
  const [form, setForm] = useState<Partial<StoreEntry>>({});

  const entries: (StoreEntry & { index: number })[] = (data?.entries || []).map(
    (e: StoreEntry, i: number) => ({ ...e, index: i })
  );

  const resetForm = () => {
    setForm({});
    setEditingIndex(null);
    setShowModal(false);
  };

  const openAdd = () => {
    setForm({ tab: "featured", btn: "", name: "", rttx: "", description: "", texture1: "interface/store/icon.rttex", texture2: "interface/store/icon.rttex", cost: 0, items: "" });
    setEditingIndex(null);
    setShowModal(true);
  };

  const openEdit = (entry: StoreEntry & { index: number }) => {
    setForm({ ...entry });
    setEditingIndex(entry.index);
    setShowModal(true);
  };

  const handleSave = async () => {
    if (!form.name) return;
    const payload = {
      action: editingIndex !== null ? "update" : "add",
      data: editingIndex !== null ? { ...form, index: editingIndex } : form,
    };
    await fetch("/api/store", {
      method: "PATCH",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
    });
    resetForm();
    mutate();
  };

  const handleDelete = async (index: number, name: string) => {
    if (!confirm(`Delete "${name}" from store?`)) return;
    await fetch("/api/store", {
      method: "PATCH",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ action: "remove", data: { index } }),
    });
    mutate();
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
          <Store size={14} className="text-white/40" />
          <h2 className="text-sm font-medium text-white/70">
            Store Manager ({entries.length} items)
          </h2>
        </div>
        <div className="flex items-center gap-2">
          <button onClick={() => mutate()} className="p-1.5 text-white/30 hover:text-white/60 transition-colors">
            <RefreshCw size={13} />
          </button>
          <button
            onClick={openAdd}
            className="flex items-center gap-1 px-2 py-1 text-[10px] rounded-md bg-white/[0.08] text-white/60 hover:bg-white/[0.12] transition-colors"
          >
            <Plus size={10} />
            Add Item
          </button>
        </div>
      </div>

      {/* Store Table */}
      <div className="overflow-x-auto rounded-lg border border-white/[0.06]">
        <table className="w-full text-xs">
          <thead>
            <tr className="text-white/30 border-b border-white/[0.06] bg-white/[0.01]">
              <th className="text-left py-2 px-3 font-normal">#</th>
              <th className="text-left py-2 px-3 font-normal">Tab</th>
              <th className="text-left py-2 px-3 font-normal">Name</th>
              <th className="text-left py-2 px-3 font-normal">Btn Text</th>
              <th className="text-right py-2 px-3 font-normal">Cost</th>
              <th className="text-left py-2 px-3 font-normal">Items</th>
              <th className="text-right py-2 px-3 font-normal">Actions</th>
            </tr>
          </thead>
          <tbody>
            {entries.map((entry, i) => (
              <motion.tr
                key={i}
                initial={{ opacity: 0 }}
                animate={{ opacity: 1 }}
                transition={{ delay: i * 0.01 }}
                className="border-b border-white/[0.03] hover:bg-white/[0.02]"
              >
                <td className="py-1.5 px-3 text-white/25 font-mono">{i + 1}</td>
                <td className="py-1.5 px-3 text-white/40">{entry.tab}</td>
                <td className="py-1.5 px-3 text-white/70">{entry.name}</td>
                <td className="py-1.5 px-3 text-white/40">{entry.btn || "-"}</td>
                <td className="py-1.5 px-3 text-right text-white/60 font-mono">{entry.cost?.toLocaleString()}</td>
                <td className="py-1.5 px-3 text-white/35 font-mono text-[10px]">{entry.items || "-"}</td>
                <td className="py-1.5 px-3 text-right">
                  <div className="flex gap-0.5 justify-end">
                    <button
                      onClick={() => openEdit(entry)}
                      className="p-1 text-white/30 hover:text-amber-400 transition-colors"
                    >
                      <Edit3 size={11} />
                    </button>
                    <button
                      onClick={() => handleDelete(entry.index, entry.name)}
                      className="p-1 text-white/30 hover:text-red-400 transition-colors"
                    >
                      <Trash2 size={11} />
                    </button>
                  </div>
                </td>
              </motion.tr>
            ))}
          </tbody>
        </table>
        {entries.length === 0 && (
          <div className="py-8 text-center text-white/30 text-sm">No store items</div>
        )}
      </div>

      {/* Add/Edit Modal */}
      <AnimatePresence>
        {showModal && (
          <motion.div
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
            className="fixed inset-0 z-50 flex items-center justify-center bg-black/50"
            onClick={resetForm}
          >
            <motion.div
              initial={{ scale: 0.95, opacity: 0 }}
              animate={{ scale: 1, opacity: 1 }}
              exit={{ scale: 0.95, opacity: 0 }}
              className="bg-[#1a1715] border border-white/[0.1] rounded-lg p-5 max-w-lg w-full mx-4 max-h-[85vh] overflow-y-auto"
              onClick={(e) => e.stopPropagation()}
            >
              <div className="flex items-center justify-between mb-4">
                <h3 className="text-sm font-semibold text-white/90">
                  {editingIndex !== null ? "Edit Store Item" : "Add Store Item"}
                </h3>
                <button onClick={resetForm} className="text-white/30 hover:text-white/60">
                  <X size={16} />
                </button>
              </div>

              <div className="space-y-3">
                <FormField label="Tab" value={form.tab || ""} onChange={(v) => setForm({ ...form, tab: v })} />
                <FormField label="Button Text" value={form.btn || ""} onChange={(v) => setForm({ ...form, btn: v })} />
                <FormField label="Name" value={form.name || ""} onChange={(v) => setForm({ ...form, name: v })} required />
                <div>
                  <label className="text-[10px] text-white/30 mb-1 block">Description</label>
                  <textarea
                    value={form.description || ""}
                    onChange={(e) => setForm({ ...form, description: e.target.value })}
                    rows={2}
                    className="w-full bg-white/[0.03] border border-white/[0.08] rounded-md px-2 py-1.5 text-xs text-white/70 placeholder:text-white/20 focus:outline-none focus:border-white/[0.15] resize-none"
                  />
                </div>
                <FormField label="RTTX" value={form.rttx || ""} onChange={(v) => setForm({ ...form, rttx: v })} />
                <FormField label="Texture 1" value={form.texture1 || ""} onChange={(v) => setForm({ ...form, texture1: v })} />
                <FormField label="Texture 2" value={form.texture2 || ""} onChange={(v) => setForm({ ...form, texture2: v })} />
                <div>
                  <label className="text-[10px] text-white/30 mb-1 block">Cost</label>
                  <input
                    type="number"
                    value={form.cost || 0}
                    onChange={(e) => setForm({ ...form, cost: parseInt(e.target.value) || 0 })}
                    className="w-full bg-white/[0.03] border border-white/[0.08] rounded-md px-2 py-1.5 text-xs text-white/70 focus:outline-none focus:border-white/[0.15]"
                  />
                </div>
                <div>
                  <label className="text-[10px] text-white/30 mb-1 block">Items (id:count,id:count)</label>
                  <input
                    type="text"
                    value={form.items || ""}
                    onChange={(e) => setForm({ ...form, items: e.target.value })}
                    placeholder="18:1,32:1"
                    className="w-full bg-white/[0.03] border border-white/[0.08] rounded-md px-2 py-1.5 text-xs text-white/70 placeholder:text-white/20 focus:outline-none focus:border-white/[0.15] font-mono"
                  />
                </div>
              </div>

              <button
                onClick={handleSave}
                disabled={!form.name}
                className="w-full mt-4 flex items-center justify-center gap-1.5 px-3 py-2 text-xs rounded-md bg-white/[0.08] text-white/70 hover:bg-white/[0.12] hover:text-white/90 disabled:opacity-30 transition-all"
              >
                <Save size={12} />
                {editingIndex !== null ? "Update Item" : "Add to Store"}
              </button>
            </motion.div>
          </motion.div>
        )}
      </AnimatePresence>
    </motion.div>
  );
}

function FormField({ label, value, onChange, required }: { label: string; value: string; onChange: (v: string) => void; required?: boolean }) {
  return (
    <div>
      <label className="text-[10px] text-white/30 mb-1 block">{label}{required ? " *" : ""}</label>
      <input
        type="text"
        value={value}
        onChange={(e) => onChange(e.target.value)}
        className="w-full bg-white/[0.03] border border-white/[0.08] rounded-md px-2 py-1.5 text-xs text-white/70 focus:outline-none focus:border-white/[0.15]"
      />
    </div>
  );
}