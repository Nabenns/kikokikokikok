#!/usr/bin/env bash
# Entrypoint for a per-tenant GrowServer instance.
set -euo pipefail

log() { echo "[gtps-entrypoint] $*"; }

: "${DATABASE_URL:?DATABASE_URL must be set (postgresql://user:***@host:port/db)}"
JWT_SECRET="${JWT_SECRET:-gtps-$(date +%s)-$RANDOM}"
DISCORD_BOT_TOKEN="${DISCORD_BOT_TOKEN:-disabled}"

# --- 1. Wait for Postgres ---
DB_HOST=$(printf '%s' "$DATABASE_URL" | sed -E 's#.*@([^:/]+).*#\1#')
DB_PORT=$(printf '%s' "$DATABASE_URL" | sed -E 's#.*@[^:/]+:([0-9]+).*#\1#')
case "$DB_PORT" in (*[!0-9]*|'') DB_PORT=5432 ;; esac
log "waiting for Postgres at $DB_HOST:$DB_PORT ..."
until node -e "const net=require('net');const s=net.createConnection($DB_PORT,'$DB_HOST');s.on('connect',()=>{process.exit(0)});s.on('error',()=>{process.exit(1)});setTimeout(()=>process.exit(1),2000)" 2>/dev/null; do
  sleep 2
done
log "Postgres reachable"

# --- 2. Write .env (if not mounted externally) ---
if [ ! -f /app/.env ]; then
  {
    printf 'DATABASE_URL=%s\n' "$DATABASE_URL"
        printf 'JWT_SECRET=%s\n' "$JWT_SECRET"
        printf 'DISCORD_BOT_TOKEN=%s\n' "$DISCORD_BOT_TOKEN"
  } > /app/.env
  log ".env written"
else
  log ".env already present (mounted), using it"
fi

# --- 3. Setup (idempotent) ---
# Skip drizzle-kit push (TUI prompt hangs non-interactive; schema already applied
# manually via `docker exec ... drizzle-kit push`). Only run setup.ts which is
# safe to re-run (items.dat download, item-info, seeds).
log "running setup ..."
export CI=true TURBO_UI=false
echo "" | pnpm --filter login-page run setup
echo "" | pnpm --filter server exec tsx scripts/setup.ts
log "setup complete"

# --- 4. Start server ---
log "starting GrowServer (pnpm dev) ..."
exec pnpm dev
