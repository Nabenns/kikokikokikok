#!/bin/bash
set -euo pipefail

# Gurotopia backup script
BACKUP_DIR="/root/gtps-host/gurotopia-runtime/backups"
TS=$(date +%Y%m%d_%H%M%S)
DST="${BACKUP_DIR}/gurotopia_${TS}"

mkdir -p "${DST}"

# 1. Database dump (peer table = accounts)
docker exec gurotopia-mariadb mariadb -uroot -p'ukEhT<ZM3~&t)jI{' gurotopia -e "SELECT * FROM peer;" > "${DST}/peer_table.tsv" 2>/dev/null
docker exec gurotopia-mariadb mariadb-dump -uroot -p'ukEhT<ZM3~&t)jI{' gurotopia peer > "${DST}/gurotopia_db.sql" 2>/dev/null

# 2. server_data.php config
cp /root/gtps-host/gurotopia-runtime/config/server_data.php "${DST}/"

# 3. Caddyfile (current working config)
cp /etc/caddy/Caddyfile "${DST}/Caddyfile.bak"

# 4. Source patches (only modified files)
cp /root/gtps-host/gurotopia-runtime/src/include/https/https.cpp "${DST}/https.cpp.patched"
cp /root/gtps-host/gurotopia-runtime/src/include/database/database.cpp "${DST}/database.cpp.patched"
cp /root/gtps-host/gurotopia-runtime/src/include/action/tankIDName.cpp "${DST}/tankIDName.cpp.patched"

# 5. items.dat + store
cp /root/gtps-host/gurotopia-runtime/src/items.dat "${DST}/" 2>/dev/null || true
cp /root/gtps-host/gurotopia-runtime/src/resources/store.txt "${DST}/" 2>/dev/null || true

# 6. docker-compose.yml
cp /root/gtps-host/gurotopia-runtime/docker-compose.yml "${DST}/"
cp /root/gtps-host/gurotopia-runtime/infra/docker-compose.mariadb.yml "${DST}/"

# Compress
tar czf "${DST}.tar.gz" -C "${BACKUP_DIR}" "gurotopia_${TS}"
rm -rf "${DST}"

# Cleanup: keep only last 7 backups
ls -t "${BACKUP_DIR}"/gurotopia_*.tar.gz 2>/dev/null | tail -n +8 | xargs rm -f 2>/dev/null || true

echo "✅ Backup created: ${DST}.tar.gz ($(du -sh "${DST}.tar.gz" | cut -f1))"
