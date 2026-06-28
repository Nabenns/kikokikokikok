"""
Gurotopia Dashboard - FastAPI backend
Serves realtime server stats, player list, logs, account management.
"""
import asyncio
import subprocess
import json
import time
import re
from datetime import datetime
from collections import deque
from pathlib import Path

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.staticfiles import StaticFiles

app = FastAPI(title="Gurotopia Dashboard")

# ─── Config ───────────────────────────────────────────────
CONTAINER = "gtps-gurotopia"
DB_CONTAINER = "gurotopia-mariadb"
DB_PASS = "ukEhT<ZM3~&t)jI{"
DB_NAME = "gurotopia"
LOG_BUFFER = deque(maxlen=200)

# ─── Helpers ──────────────────────────────────────────────
def run_cmd(cmd: str, timeout: int = 5) -> str:
    try:
        r = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=timeout)
        return r.stdout.strip() or r.stderr.strip()
    except Exception as e:
        return f"Error: {e}"

def get_container_status(name: str) -> dict:
    raw = run_cmd(f'docker ps -a --filter name={name} --format "{{{{.Status}}}}|{{{{.Ports}}}}"')
    if not raw or "Error" in raw:
        return {"running": False, "status": "not found", "ports": ""}
    status, ports = raw.split("|", 1)
    return {
        "running": "Up" in status,
        "status": status,
        "ports": ports
    }

def get_db_accounts() -> list:
    raw = run_cmd(f'docker exec {DB_CONTAINER} mariadb -uroot -p\'{DB_PASS}\' {DB_NAME} -e "SELECT uid,growid,created_at FROM peer ORDER BY uid;" --batch 2>/dev/null')
    if not raw or "Error" in raw:
        return []
    lines = raw.strip().split("\n")[1:]  # skip header
    accounts = []
    for line in lines:
        parts = line.split("\t")
        if len(parts) >= 3:
            accounts.append({"uid": parts[0], "growid": parts[1], "created_at": parts[2]})
    return accounts

def get_player_count() -> int:
    raw = run_cmd(f'docker exec {CONTAINER} cat /proc/net/udp 2>/dev/null')
    if not raw:
        return 0
    # Count active UDP connections (excluding the listening socket)
    lines = [l for l in raw.strip().split("\n")[1:] if l.strip()]
    # Filter for established connections (state 01 = ESTABLISHED)
    established = [l for l in lines if l.strip().split()[3] == "01"]
    return len(established)

def get_recent_logs(n: int = 50) -> list:
    raw = run_cmd(f'docker logs {CONTAINER} --tail {n} 2>&1')
    if not raw:
        return []
    return raw.strip().split("\n")

def get_uptime_seconds() -> float:
    raw = run_cmd(f'docker ps --filter name={CONTAINER} --format "{{{{.Status}}}}"')
    if not raw or "Up" not in raw:
        return 0
    # Parse "Up 2 minutes" / "Up 3 hours" / "Up 5 seconds"
    m = re.match(r'Up (\d+) (\w+)', raw)
    if not m:
        return 0
    val, unit = int(m.group(1)), m.group(2)
    multipliers = {"second": 1, "seconds": 1, "minute": 60, "minutes": 60, "hour": 3600, "hours": 3600, "day": 86400, "days": 86400}
    return val * multipliers.get(unit, 0)

def get_server_data() -> dict:
    raw = run_cmd(f'curl -sk -X POST "https://127.0.0.1:8444/growtopia/server_data.php" -d "protocol=0&game_version=5.39&f=1" 2>/dev/null')
    if not raw:
        return {}
    data = {}
    for line in raw.strip().split("\n"):
        if "|" in line and not line.startswith("RTEND"):
            k, v = line.split("|", 1)
            data[k] = v
    return data

# ─── API Routes ───────────────────────────────────────────
@app.get("/api/status")
async def api_status():
    return {
        "server": get_container_status(CONTAINER),
        "mariadb": get_container_status(DB_CONTAINER),
        "uptime_seconds": get_uptime_seconds(),
        "player_count": get_player_count(),
        "server_data": get_server_data(),
        "timestamp": datetime.now().isoformat()
    }

@app.get("/api/accounts")
async def api_accounts():
    return {"accounts": get_db_accounts()}

@app.get("/api/logs")
async def api_logs(n: int = 50):
    return {"logs": get_recent_logs(n)}

@app.post("/api/restart")
async def api_restart():
    result = run_cmd(f'docker restart {CONTAINER}', timeout=30)
    return {"result": result or "restarted"}

@app.post("/api/backup")
async def api_backup():
    result = run_cmd('bash /root/gtps-host/gurotopia-runtime/backup.sh', timeout=30)
    return {"result": result}

# ─── WebSocket for realtime ───────────────────────────────
@app.websocket("/ws")
async def ws_endpoint(websocket: WebSocket):
    await websocket.accept()
    try:
        while True:
            data = {
                "server": get_container_status(CONTAINER),
                "mariadb": get_container_status(DB_CONTAINER),
                "uptime_seconds": get_uptime_seconds(),
                "player_count": get_player_count(),
                "timestamp": datetime.now().isoformat()
            }
            await websocket.send_json(data)
            await asyncio.sleep(3)
    except WebSocketDisconnect:
        pass

# ─── Dashboard HTML ───────────────────────────────────────
@app.get("/", response_class=HTMLResponse)
async def dashboard():
    return HTML_PAGE

# ─── HTML Page ────────────────────────────────────────────
HTML_PAGE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Gurotopia Dashboard</title>
<style>
:root {
  --bg: #0f0f0f;
  --card: #1a1a2e;
  --accent: #16213e;
  --primary: #0f3460;
  --text: #e0e0e0;
  --green: #4caf50;
  --red: #f44336;
  --yellow: #ffc107;
  --blue: #2196f3;
  --border: #2a2a4a;
}
* { margin: 0; padding: 0; box-sizing: border-box; }
body {
  font-family: 'Segoe UI', system-ui, sans-serif;
  background: var(--bg);
  color: var(--text);
  min-height: 100vh;
}
.header {
  background: linear-gradient(135deg, var(--accent), var(--primary));
  padding: 20px 30px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  border-bottom: 2px solid var(--border);
}
.header h1 { font-size: 1.5rem; }
.header .sub { color: #888; font-size: 0.85rem; margin-top: 2px; }
.status-dot {
  width: 10px; height: 10px; border-radius: 50%;
  display: inline-block; margin-right: 6px;
}
.status-dot.online { background: var(--green); box-shadow: 0 0 6px var(--green); }
.status-dot.offline { background: var(--red); }
.container { padding: 20px 30px; max-width: 1200px; margin: 0 auto; }
.grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
  gap: 16px;
  margin-bottom: 20px;
}
.card {
  background: var(--card);
  border: 1px solid var(--border);
  border-radius: 12px;
  padding: 20px;
}
.card h3 { font-size: 0.8rem; text-transform: uppercase; color: #888; margin-bottom: 12px; letter-spacing: 1px; }
.card .value { font-size: 2rem; font-weight: 700; }
.card .sub { color: #888; font-size: 0.85rem; margin-top: 4px; }
.card .label { color: #aaa; font-size: 0.9rem; }
.stats-row { display: flex; gap: 30px; flex-wrap: wrap; }
.stats-row .stat { text-align: center; }
.stats-row .stat .num { font-size: 1.8rem; font-weight: 700; }
.stats-row .stat .lbl { font-size: 0.75rem; color: #888; text-transform: uppercase; }
.btn {
  background: var(--primary);
  color: var(--text);
  border: 1px solid var(--border);
  padding: 8px 16px;
  border-radius: 8px;
  cursor: pointer;
  font-size: 0.85rem;
  transition: 0.2s;
}
.btn:hover { background: #1a4a7a; }
.btn.danger { background: #4a1520; }
.btn.danger:hover { background: #6a1f2a; }
.btn-row { display: flex; gap: 10px; margin: 15px 0; }
table { width: 100%; border-collapse: collapse; }
th, td { text-align: left; padding: 10px 8px; border-bottom: 1px solid var(--border); font-size: 0.85rem; }
th { color: #888; text-transform: uppercase; font-size: 0.75rem; }
.log-box {
  background: #0a0a0a;
  border: 1px solid var(--border);
  border-radius: 8px;
  padding: 12px;
  max-height: 400px;
  overflow-y: auto;
  font-family: 'Consolas', monospace;
  font-size: 0.78rem;
  line-height: 1.5;
}
.log-line { white-space: pre-wrap; word-break: break-all; }
.log-line.login { color: var(--green); }
.log-line.quit { color: var(--yellow); }
.log-line.error { color: var(--red); }
.log-line.action { color: var(--blue); }
.log-line.other { color: #999; }
.section-title { font-size: 1.1rem; margin: 20px 0 10px; display: flex; align-items: center; gap: 8px; }
.refresh-btn { font-size: 0.75rem; padding: 4px 10px; }
.pill { display: inline-block; padding: 2px 8px; border-radius: 12px; font-size: 0.7rem; font-weight: 600; }
.pill.online { background: #1b3a1b; color: var(--green); }
.pill.offline { background: #3a1b1b; color: var(--red); }
</style>
</head>
<body>
<div class="header">
  <div>
    <h1>🎮 Gurotopia Dashboard</h1>
    <div class="sub" id="lastUpdate">Loading...</div>
  </div>
  <div>
    <span class="status-dot" id="headerDot"></span>
    <span id="headerStatus">Connecting...</span>
  </div>
</div>

<div class="container">
  <div class="grid">
    <div class="card">
      <h3>Server Status</h3>
      <div class="value" id="serverStatus">—</div>
      <div class="sub" id="serverPorts">—</div>
      <div class="sub" id="serverUptime">—</div>
    </div>
    <div class="card">
      <h3>MariaDB</h3>
      <div class="value" id="dbStatus">—</div>
      <div class="sub" id="dbPorts">—</div>
    </div>
    <div class="card">
      <h3>Players Online</h3>
      <div class="value" id="playerCount">0</div>
      <div class="sub">UDP active connections</div>
    </div>
    <div class="card">
      <h3>Server Config</h3>
      <div class="label" id="serverIP">IP: —</div>
      <div class="label" id="serverPort">Port: —</div>
      <div class="label" id="serverLogin">LoginURL: —</div>
    </div>
  </div>

  <div class="card">
    <h3>Quick Actions</h3>
    <div class="btn-row">
      <button class="btn" onclick="refreshAll()">🔄 Refresh</button>
      <button class="btn" onclick="restartServer()">🔁 Restart Server</button>
      <button class="btn" onclick="backupServer()">💾 Backup Now</button>
      <button class="btn" onclick="refreshLogs()">📋 Refresh Logs</button>
    </div>
  </div>

  <div class="section-title">👤 Accounts</div>
  <div class="card">
    <table>
      <thead><tr><th>UID</th><th>GrowID</th><th>Created</th></tr></thead>
      <tbody id="accountsBody"></tbody>
    </table>
  </div>

  <div class="section-title">📜 Server Logs <button class="btn refresh-btn" onclick="refreshLogs()">↻</button></div>
  <div class="log-box" id="logBox">Loading logs...</div>
</div>

<script>
let ws = null;
function connectWS() {
  const proto = location.protocol === 'https:' ? 'wss' : 'ws';
  ws = new WebSocket(`${proto}://${location.host}/ws`);
  ws.onmessage = (e) => {
    const d = JSON.parse(e.data);
    updateStatus(d);
  };
  ws.onclose = () => {
    document.getElementById('headerDot').className = 'status-dot offline';
    document.getElementById('headerStatus').textContent = 'Disconnected';
    setTimeout(connectWS, 3000);
  };
}

function updateStatus(d) {
  const srv = d.server;
  const db = d.mariadb;
  const online = srv.running;
  document.getElementById('headerDot').className = `status-dot ${online ? 'online' : 'offline'}`;
  document.getElementById('headerStatus').textContent = online ? 'Online' : 'Offline';
  document.getElementById('serverStatus').innerHTML = `<span class="pill ${online?'online':'offline'}">${srv.status}</span>`;
  document.getElementById('serverPorts').textContent = srv.ports || '';
  const up = d.uptime_seconds;
  const h = Math.floor(up/3600), m = Math.floor((up%3600)/60), s = Math.floor(up%60);
  document.getElementById('serverUptime').textContent = `Uptime: ${h}h ${m}m ${s}s`;
  document.getElementById('dbStatus').innerHTML = `<span class="pill ${db.running?'online':'offline'}">${db.status}</span>`;
  document.getElementById('dbPorts').textContent = db.ports || '';
  document.getElementById('playerCount').textContent = d.player_count;
  document.getElementById('lastUpdate').textContent = `Last update: ${new Date(d.timestamp).toLocaleString()}`;
}

async function refreshAll() {
  const [statusRes, accountsRes] = await Promise.all([
    fetch('/api/status').then(r=>r.json()),
    fetch('/api/accounts').then(r=>r.json())
  ]);
  updateStatus(statusRes);
  const tbody = document.getElementById('accountsBody');
  tbody.innerHTML = (accountsRes.accounts||[]).map(a =>
    `<tr><td>${a.uid}</td><td>${a.growid}</td><td>${a.created_at}</td></tr>`
  ).join('') || '<tr><td colspan=3>No accounts</td></tr>';
  if (statusRes.server_data) {
    document.getElementById('serverIP').textContent = `IP: ${statusRes.server_data.server||'—'}`;
    document.getElementById('serverPort').textContent = `Port: ${statusRes.server_data.port||'—'}`;
    document.getElementById('serverLogin').textContent = `LoginURL: ${statusRes.server_data.loginurl||'(empty)'}`;
  }
}

async function refreshLogs() {
  const res = await fetch('/api/logs?n=80');
  const d = await res.json();
  const box = document.getElementById('logBox');
  box.innerHTML = (d.logs||[]).map(line => {
    let cls = 'other';
    if (/tankIDName|login|enter_game/i.test(line)) cls = 'login';
    else if (/quit|disconnect/i.test(line)) cls = 'quit';
    else if (/error|fail|exception/i.test(line)) cls = 'error';
    else if (/action\|/i.test(line)) cls = 'action';
    return `<div class="log-line ${cls}">${line.replace(/</g,'&lt;')}</div>`;
  }).join('') || 'No logs';
  box.scrollTop = 0;
}

async function restartServer() {
  if (!confirm('Restart Gurotopia server?')) return;
  const res = await fetch('/api/restart', {method:'POST'});
  const d = await res.json();
  alert(d.result || 'Done');
  refreshAll();
}

async function backupServer() {
  const res = await fetch('/api/backup', {method:'POST'});
  const d = await res.json();
  alert(d.result || 'Done');
}

connectWS();
refreshAll();
refreshLogs();
setInterval(refreshAll, 15000);
setInterval(refreshLogs, 10000);
</script>
</body>
</html>
"""
