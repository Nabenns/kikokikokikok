# Gurotopia GTPS

Growtopia Private Server — built from scratch in C++23.

**Domain:** `gtps.bensserver.cloud`
**Login:** `login-gurotopia.vercel.app`

## Architecture

```
gurotopia-runtime/       # C++ server (ENet + MariaDB + OpenSSL)
  src/
    main.cpp             # Entry point — ENet host, HTTPS thread, game loop
    include/
      database/          # MariaDB client, hStmt RAII, world persistence
      action/            # 35+ player actions (buy, cook, combine, dialog)
      commands/          # 43 admin commands (ban, mute, give, setrole, etc.)
      on/                # 34 packet handlers (spawn, action, clothing, etc.)
      event_type/        # ENet connect/receive/disconnect dispatch
      http/              # HTTPS listener — admin API (online, broadcast, items)
      automate/          # Auto-save, backup, holiday checker, hardening
      manager/           # Item decode, store parser
      state/             # Tile/movement/activate/disconnect state handlers
      utils/             # Color, text, variantlist, world pool, crypto
  resources/
    items.dat            # 16K+ items database
    store.txt            # 109 store entries

web/                     # Next.js 16 Admin Dashboard
  app/
    page.tsx             # Main dashboard (11 sections)
    api/                 # 26 API routes (players, worlds, economy, audit, etc.)
    components/          # 10 UI components (tiered dashboard layout)

provisioner/             # Infrastructure scripts
infra/                   # Docker Compose files
```

## Tech Stack

| Layer | Tech |
|-------|------|
| **Server** | C++23, ENet, OpenSSL, MariaDB C API |
| **Game Protocol** | VariantList binary, CRC32 compress |
| **Dashboard** | Next.js 16, TypeScript, shadcn/ui, recharts |
| **Proxy** | Caddy (Let's Encrypt) |
| **Deploy** | Docker, Docker Compose, systemd |

## Game Features

- 35+ player actions (buy, sell, cook, combine, splice, etc.)
- 43 admin commands (+ audit logging)
- World system with lazy load/save + auto-cleanup
- Guild system
- Friends & trade
- Mailbox
- Pets & quests
- Achievements
- Daily rewards & titles
- Store with 100+ items

## Dashboard Features

- Live player monitor + broadcast
- Role manager
- Economy overview (gems distribution)
- World explorer
- Guild manager
- Item browser (16K items)
- Store manager (live edit)
- Mailbox viewer
- Audit log + report queue
- Player detail (inventory, clothing, friends, pets)
- Analytics (growth chart, activity heatmap)
- Backup manager + live console
- Server config panel + restart

## Ports

| Port | Protocol | Service |
|------|----------|---------|
| 17091 | UDP | ENet game server |
| 8444 | TCP | HTTPS admin API |
| 8088 | TCP | Next.js dashboard (systemd) |
| 3307 | TCP | MariaDB |

## Quick Start

```bash
# Server
cd gurotopia-runtime/src
make -j$(nproc)
docker cp main.out gtps-gurotopia:/app/
docker restart gtps-gurotopia

# Dashboard
cd web
npm install
npm run build
systemctl restart gtps-dashboard
```

## License

Private — Gurotopia (c) 2024-2026