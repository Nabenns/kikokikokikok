# GTPS hosting runtime image for StileDevs/GrowServer.
#
# WHY a custom Dockerfile (the upstream one doesn't work for hosting):
#   - Upstream Dockerfile only does `pnpm install` + `CMD pnpm dev`. It NEVER
#     runs `pnpm run setup`, so the DB schema is never pushed, items.dat is
#     never downloaded, and SSL certs are never generated -> the server crashes
#     on first run.
#   - setup runs `drizzle-kit push`, which needs a LIVE Postgres. That can't
#     happen at image-build time (no DB yet), so we run setup in the ENTRYPOINT
#     at container start, after waiting for Postgres.
#
# Build context = growserver-runtime/ (so both the cloned repo in src/ AND
# entrypoint.sh are inside the context). Build from gtps-host root:
#   docker build -t gtps-growserver:latest -f growserver-runtime/Dockerfile growserver-runtime
#
# The image is GENERIC (no tenant data baked in). Per-tenant differences are
# injected at run time:
#   - config.toml  -> bind-mount onto /app/packages/config/config.toml
#   - DATABASE_URL -> env var (points at the tenant's DB on shared Postgres)
#   - game UDP port -> docker port map hostPort/udp:hostPort/udp (must match the
#                      `ports = [<gamePort>]` in the mounted config.toml)
FROM node:22-bookworm-slim

# certutil (libnss3-tools) lets mkcert install its local CA; ca-certificates +
# curl for the setup downloads (items.dat, website bundle, mkcert binary).
# netcat for the Postgres wait-loop in the entrypoint.
RUN apt-get update && apt-get install -y --no-install-recommends \
      ca-certificates curl libnss3-tools netcat-openbsd \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY src/ /app/

# corepack reads the repo's packageManager field (pnpm@9) and pins it.
RUN corepack enable && corepack prepare pnpm@9.0.0 --activate
RUN pnpm install --frozen-lockfile

# ENet game port (UDP) + login/web (TCP). Actual published port chosen per
# tenant at `docker run`; EXPOSE is documentation only.
EXPOSE 8080/tcp
EXPOSE 443/tcp
EXPOSE 17091/udp

COPY --chmod=0755 entrypoint.sh /usr/local/bin/gtps-entrypoint.sh
ENTRYPOINT ["/usr/local/bin/gtps-entrypoint.sh"]
