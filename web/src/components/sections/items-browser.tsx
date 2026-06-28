"use client";

import { useState, useEffect, useCallback } from "react";
import useSWR from "swr";
import { motion, AnimatePresence } from "framer-motion";
import { Search, ChevronLeft, ChevronRight, Package, RefreshCw } from "lucide-react";

const fetcher = (url: string) => fetch(url).then((r) => r.json());

interface Item {
  id: number;
  name: string;
  type: string;
  rarity: string;
  category: string;
  properties?: Record<string, unknown>;
  collision?: number;
  hits?: number;
  cloth_type?: string;
  grow_time?: number;
  [key: string]: unknown;
}

function rarityBadge(rarity: string) {
  const colors: Record<string, string> = {
    "1": "bg-white/10 text-white/40",
    "2": "bg-emerald-500/10 text-emerald-400",
    "3": "bg-blue-500/10 text-blue-400",
    "4": "bg-violet-500/10 text-violet-400",
    "5": "bg-amber-500/10 text-amber-400",
    "999": "bg-red-500/10 text-red-400",
    legendary: "bg-amber-500/10 text-amber-400",
    rare: "bg-blue-500/10 text-blue-400",
    common: "bg-white/10 text-white/40",
    epic: "bg-violet-500/10 text-violet-400",
  };
  const r = rarity?.toString().toLowerCase() || "";
  return colors[r] || "bg-white/5 text-white/30";
}

export function ItemsBrowser() {
  const [search, setSearch] = useState("");
  const [debouncedSearch, setDebouncedSearch] = useState("");
  const [page, setPage] = useState(1);
  const [selectedItem, setSelectedItem] = useState<Item | null>(null);

  useEffect(() => {
    const t = setTimeout(() => {
      setDebouncedSearch(search);
      setPage(1);
    }, 300);
    return () => clearTimeout(t);
  }, [search]);

  const url = `/api/items?page=${page}${debouncedSearch ? `&search=${encodeURIComponent(debouncedSearch)}` : ""}`;
  const { data, error, mutate } = useSWR(url, fetcher, { dedupingInterval: 2000 });

  const items: Item[] = data?.items || [];
  const totalPages = data?.totalPages || 0;

  return (
    <motion.div
      initial={{ opacity: 0, y: 12 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.3 }}
      className="space-y-4"
    >
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-2">
          <Package size={14} className="text-white/40" />
          <h2 className="text-sm font-medium text-white/70">
            Items Browser {data?.total ? `(${data.total.toLocaleString()} items)` : ""}
          </h2>
        </div>
        <button onClick={() => mutate()} className="p-1.5 text-white/30 hover:text-white/60 transition-colors">
          <RefreshCw size={13} />
        </button>
      </div>

      {/* Search */}
      <div className="relative">
        <Search size={12} className="absolute left-2.5 top-1/2 -translate-y-1/2 text-white/20" />
        <input
          type="text"
          value={search}
          onChange={(e) => setSearch(e.target.value)}
          placeholder="Search items by name..."
          className="w-full bg-white/[0.03] border border-white/[0.08] rounded-md pl-8 pr-3 py-2 text-xs text-white/70 placeholder:text-white/20 focus:outline-none focus:border-white/[0.15]"
        />
      </div>

      {/* Items Table */}
      <div className="overflow-x-auto rounded-lg border border-white/[0.06]">
        <table className="w-full text-xs">
          <thead>
            <tr className="text-white/30 border-b border-white/[0.06] bg-white/[0.01]">
              <th className="text-left py-2 px-3 font-normal w-16">ID</th>
              <th className="text-left py-2 px-3 font-normal">Name</th>
              <th className="text-left py-2 px-3 font-normal">Type</th>
              <th className="text-left py-2 px-3 font-normal">Rarity</th>
              <th className="text-left py-2 px-3 font-normal">Category</th>
            </tr>
          </thead>
          <tbody>
            {items.map((item, i) => (
              <motion.tr
                key={item.id}
                initial={{ opacity: 0 }}
                animate={{ opacity: 1 }}
                transition={{ delay: i * 0.003 }}
                onClick={() => setSelectedItem(item)}
                className="border-b border-white/[0.03] hover:bg-white/[0.03] cursor-pointer transition-colors"
              >
                <td className="py-1.5 px-3 text-white/30 font-mono">{item.id}</td>
                <td className="py-1.5 px-3 text-white/70">{item.name}</td>
                <td className="py-1.5 px-3 text-white/40">{item.type || "-"}</td>
                <td className="py-1.5 px-3">
                  <span className={`text-[10px] px-1.5 py-0.5 rounded ${rarityBadge(item.rarity)}`}>
                    {item.rarity || "?"}
                  </span>
                </td>
                <td className="py-1.5 px-3 text-white/40">{item.category || "-"}</td>
              </motion.tr>
            ))}
          </tbody>
        </table>
        {items.length === 0 && !data && (
          <div className="py-8 text-center text-white/30 text-sm animate-pulse">Loading...</div>
        )}
        {items.length === 0 && data && (
          <div className="py-8 text-center text-white/30 text-sm">No items found</div>
        )}
      </div>

      {/* Pagination */}
      {totalPages > 1 && (
        <div className="flex items-center justify-center gap-3">
          <button
            onClick={() => setPage((p) => Math.max(1, p - 1))}
            disabled={page <= 1}
            className="p-1.5 text-white/30 hover:text-white/60 disabled:opacity-20 transition-colors"
          >
            <ChevronLeft size={14} />
          </button>
          <span className="text-xs text-white/40">
            Page {page} / {totalPages}
          </span>
          <button
            onClick={() => setPage((p) => Math.min(totalPages, p + 1))}
            disabled={page >= totalPages}
            className="p-1.5 text-white/30 hover:text-white/60 disabled:opacity-20 transition-colors"
          >
            <ChevronRight size={14} />
          </button>
        </div>
      )}

      {/* Item Detail Modal */}
      <AnimatePresence>
        {selectedItem && (
          <motion.div
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
            className="fixed inset-0 z-50 flex items-center justify-center bg-black/50"
            onClick={() => setSelectedItem(null)}
          >
            <motion.div
              initial={{ scale: 0.95, opacity: 0 }}
              animate={{ scale: 1, opacity: 1 }}
              exit={{ scale: 0.95, opacity: 0 }}
              className="bg-[#1a1715] border border-white/[0.1] rounded-lg p-5 max-w-md w-full mx-4 max-h-[80vh] overflow-y-auto"
              onClick={(e) => e.stopPropagation()}
            >
              <div className="flex items-center justify-between mb-4">
                <h3 className="text-sm font-semibold text-white/90">{selectedItem.name}</h3>
                <span className="text-xs text-white/30 font-mono">ID: {selectedItem.id}</span>
              </div>

              <div className="grid grid-cols-2 gap-2 text-xs mb-3">
                <DetailRow label="Type" value={selectedItem.type} />
                <DetailRow label="Rarity" value={selectedItem.rarity} />
                <DetailRow label="Category" value={selectedItem.category} />
                {selectedItem.collision !== undefined && (
                  <DetailRow label="Collision" value={String(selectedItem.collision)} />
                )}
                {selectedItem.hits !== undefined && (
                  <DetailRow label="Hits" value={String(selectedItem.hits)} />
                )}
                {selectedItem.cloth_type && (
                  <DetailRow label="Cloth Type" value={selectedItem.cloth_type} />
                )}
                {selectedItem.grow_time !== undefined && (
                  <DetailRow label="Grow Time" value={`${selectedItem.grow_time}s`} />
                )}
              </div>

              {selectedItem.properties && Object.keys(selectedItem.properties).length > 0 && (
                <div className="mb-3">
                  <div className="text-[10px] text-white/30 mb-1.5">Properties</div>
                  <div className="bg-black/20 rounded-md p-2 text-[10px] font-mono text-white/50 max-h-[200px] overflow-y-auto">
                    {JSON.stringify(selectedItem.properties, null, 2)}
                  </div>
                </div>
              )}

              <button
                onClick={() => setSelectedItem(null)}
                className="w-full mt-2 px-3 py-1.5 text-xs rounded-md bg-white/[0.08] text-white/60 hover:bg-white/[0.12] transition-colors"
              >
                Close
              </button>
            </motion.div>
          </motion.div>
        )}
      </AnimatePresence>
    </motion.div>
  );
}

function DetailRow({ label, value }: { label: string; value?: string }) {
  return (
    <div className="bg-white/[0.03] rounded p-1.5">
      <span className="text-white/30">{label}:</span>{" "}
      <span className="text-white/60">{value || "-"}</span>
    </div>
  );
}