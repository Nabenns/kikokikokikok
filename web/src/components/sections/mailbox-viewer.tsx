"use client";

import { useState, useCallback } from "react";
import useSWR from "swr";
import { motion } from "framer-motion";
import { Mail, Trash2, RefreshCw, Search, MailCheck, MailX } from "lucide-react";
import { format, formatDistanceToNow } from "date-fns";

const fetcher = (url: string) => fetch(url).then((r) => r.json());

interface MailMessage {
  id: number;
  recipient_uid: number;
  recipient_name: string;
  sender_name: string;
  message: string;
  item_id: number;
  item_count: number;
  sent_at: string;
  is_read: number;
}

export function MailboxViewer() {
  const [recipientFilter, setRecipientFilter] = useState("");
  const [debouncedFilter, setDebouncedFilter] = useState("");
  const [deleting, setDeleting] = useState<number | null>(null);

  const url = `/api/mailbox${debouncedFilter ? `?recipient=${debouncedFilter}` : ""}`;
  const { data, error, mutate } = useSWR(url, fetcher, { refreshInterval: 15000 });

  // Debounce filter
  const handleFilterChange = useCallback((val: string) => {
    setRecipientFilter(val);
    // Use a simple setTimeout approach
    const timer = setTimeout(() => setDebouncedFilter(val), 400);
    return () => clearTimeout(timer);
  }, []);

  const handleDelete = async (id: number) => {
    if (!confirm(`Delete mail #${id}?`)) return;
    setDeleting(id);
    await fetch("/api/mailbox", {
      method: "DELETE",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ id }),
    });
    setDeleting(null);
    mutate();
  };

  const messages: MailMessage[] = data?.messages || [];

  return (
    <motion.div
      initial={{ opacity: 0, y: 12 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.3 }}
      className="space-y-4"
    >
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-2">
          <Mail size={14} className="text-white/40" />
          <h2 className="text-sm font-medium text-white/70">
            Mailbox ({data?.count ?? messages.length} messages)
          </h2>
        </div>
        <button onClick={() => mutate()} className="p-1.5 text-white/30 hover:text-white/60 transition-colors">
          <RefreshCw size={13} />
        </button>
      </div>

      {/* Filter */}
      <div className="relative">
        <Search size={12} className="absolute left-2.5 top-1/2 -translate-y-1/2 text-white/20" />
        <input
          type="text"
          value={recipientFilter}
          onChange={(e) => {
            setRecipientFilter(e.target.value);
            if (!e.target.value) setDebouncedFilter("");
            const timer = setTimeout(() => setDebouncedFilter(e.target.value), 400);
          }}
          placeholder="Filter by recipient UID..."
          className="w-full bg-white/[0.03] border border-white/[0.08] rounded-md pl-8 pr-3 py-2 text-xs text-white/70 placeholder:text-white/20 focus:outline-none focus:border-white/[0.15]"
        />
      </div>

      {/* Messages Table */}
      <div className="overflow-x-auto rounded-lg border border-white/[0.06]">
        <table className="w-full text-xs">
          <thead>
            <tr className="text-white/30 border-b border-white/[0.06] bg-white/[0.01]">
              <th className="text-left py-2 px-3 font-normal">ID</th>
              <th className="text-left py-2 px-3 font-normal">From</th>
              <th className="text-left py-2 px-3 font-normal">To</th>
              <th className="text-left py-2 px-3 font-normal">Message</th>
              <th className="text-left py-2 px-3 font-normal">Item</th>
              <th className="text-left py-2 px-3 font-normal">Sent</th>
              <th className="text-center py-2 px-3 font-normal">Read</th>
              <th className="text-right py-2 px-3 font-normal">Actions</th>
            </tr>
          </thead>
          <tbody>
            {messages.map((msg, i) => (
              <motion.tr
                key={msg.id}
                initial={{ opacity: 0 }}
                animate={{ opacity: 1 }}
                transition={{ delay: i * 0.005 }}
                className="border-b border-white/[0.03] hover:bg-white/[0.02]"
              >
                <td className="py-1.5 px-3 text-white/25 font-mono">{msg.id}</td>
                <td className="py-1.5 px-3 text-white/60">{msg.sender_name}</td>
                <td className="py-1.5 px-3 text-white/50">{msg.recipient_name}</td>
                <td className="py-1.5 px-3 text-white/40 max-w-[200px] truncate">{msg.message}</td>
                <td className="py-1.5 px-3 text-white/35 font-mono">
                  {msg.item_id > 0 ? `${msg.item_id}:${msg.item_count}` : "-"}
                </td>
                <td className="py-1.5 px-3 text-white/25">
                  {msg.sent_at ? format(new Date(msg.sent_at), "MMM dd HH:mm") : "-"}
                </td>
                <td className="py-1.5 px-3 text-center">
                  {msg.is_read ? (
                    <MailCheck size={12} className="inline text-emerald-400/60" />
                  ) : (
                    <MailX size={12} className="inline text-white/20" />
                  )}
                </td>
                <td className="py-1.5 px-3 text-right">
                  <button
                    onClick={() => handleDelete(msg.id)}
                    disabled={deleting === msg.id}
                    className="p-1 text-white/20 hover:text-red-400 transition-colors disabled:opacity-20"
                  >
                    <Trash2 size={11} />
                  </button>
                </td>
              </motion.tr>
            ))}
          </tbody>
        </table>
        {messages.length === 0 && !data && (
          <div className="py-8 text-center text-white/30 text-sm animate-pulse">Loading messages...</div>
        )}
        {messages.length === 0 && data && (
          <div className="py-8 text-center text-white/30 text-sm">
            {debouncedFilter ? "No messages for this recipient" : "No messages in mailbox"}
          </div>
        )}
      </div>
    </motion.div>
  );
}