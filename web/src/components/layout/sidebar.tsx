"use client";

import { useState, useEffect } from "react";
import { useTheme } from "next-themes";
import {
  LayoutDashboard,
  Users,
  Globe,
  FileText,
  BarChart3,
  Shield,
  Sliders,
  Terminal,
  Activity,
  TrendingUp,
  Package,
  Store,
  Settings,
  Mail,
} from "lucide-react";

const navItems = [
  { id: "overview", label: "Overview", icon: LayoutDashboard },
  { id: "players", label: "Live Players", icon: Users },
  { id: "items", label: "Items", icon: Package },
  { id: "store", label: "Store Manager", icon: Store },
  { id: "economy", label: "Economy", icon: BarChart3 },
  { id: "worlds", label: "Worlds", icon: Globe },
  { id: "moderation", label: "Moderation", icon: Shield },
  { id: "guilds", label: "Guilds", icon: Activity },
  { id: "analytics", label: "Analytics", icon: TrendingUp },
  { id: "backups", label: "Backups", icon: Sliders },
  { id: "config", label: "Config", icon: Settings },
  { id: "mailbox", label: "Mailbox", icon: Mail },
  { id: "console", label: "Console", icon: Terminal },
  { id: "logs", label: "Server Logs", icon: FileText },
  { id: "accounts", label: "Accounts", icon: Users },
];

function ThemeToggle() {
  const { theme, setTheme, resolvedTheme } = useTheme();
  const [mounted, setMounted] = useState(false);
  useEffect(() => setMounted(true), []);
  if (!mounted) return <div className="w-5 h-5" />;

  return (
    <button
      onClick={() => setTheme(resolvedTheme === "dark" ? "light" : "dark")}
      className="p-1.5 text-white/40 hover:text-white/70 hover:bg-white/5 rounded-md transition-all"
      aria-label="Toggle theme"
    >
      {resolvedTheme === "dark" ? (
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
          <circle cx="12" cy="12" r="5" /><path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42" />
        </svg>
      ) : (
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
          <path d="M21 12.79A9 9 0 1111.21 3 7 7 0 0021 12.79z" />
        </svg>
      )}
    </button>
  );
}

export function Sidebar({
  activeSection,
  onNavigate,
}: {
  activeSection: string;
  onNavigate: (id: string) => void;
}) {
  return (
    <aside className="hidden md:flex flex-col w-[200px] shrink-0 sticky top-0 h-screen overflow-y-auto">
      {/* logo */}
      <div className="pt-12 md:pt-16 pb-8">
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-2.5">
            <div className="flex h-7 w-7 items-center justify-center rounded-md bg-[#1c1917] dark:bg-white/[0.06] dark:border dark:border-white/[0.09]">
              <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" className="text-[#e7e5e4] dark:text-white/70">
                <rect x="3" y="3" width="18" height="18" rx="2" /><path d="M9 9h6v6H9z" />
              </svg>
            </div>
            <span className="text-[15px] font-semibold text-[#1c1917] dark:text-white/90 transition-colors">
              Gurotopia
            </span>
          </div>
          <ThemeToggle />
        </div>
      </div>

      {/* nav */}
      <nav className="flex-1 overflow-y-auto">
        <ul className="space-y-0.5 mb-6 pl-3">
          {navItems.map((item) => {
            const isActive = activeSection === item.id;
            const Icon = item.icon;
            return (
              <li key={item.id}>
                <button
                  onClick={() => onNavigate(item.id)}
                  className={`w-full flex items-center gap-2.5 px-3 py-1.5 text-[13px] rounded-md transition-all ${
                    isActive
                      ? "text-white/90 bg-white/[0.06] font-medium"
                      : "text-white/35 hover:text-white/60 hover:bg-white/[0.02]"
                  }`}
                >
                  <Icon size={14} className={isActive ? "text-white/60" : "text-white/25"} />
                  {item.label}
                </button>
              </li>
            );
          })}
        </ul>
      </nav>
    </aside>
  );
}