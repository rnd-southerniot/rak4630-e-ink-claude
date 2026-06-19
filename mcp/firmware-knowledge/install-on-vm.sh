#!/usr/bin/env bash
# Deploy/update the firmware-knowledge MCP server on the SIoT MCP gateway VM, and
# register it as an upstream. Re-run to refresh the served knowledge after editing docs.
#
# Run from the repo root:  mcp/firmware-knowledge/install-on-vm.sh
# Env: MCP_VM (ssh host, default "mcp-vm"), PORT (default 8002)
set -euo pipefail

VM="${MCP_VM:-mcp-vm}"
PORT="${PORT:-8002}"
REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
KNOW_DIR="/home/mcp/knowledge/rak4630-e-ink-claude"
SRV_DIR="/home/mcp/mcp-servers/firmware-knowledge"

# Use sudo only if not already root on the VM.
SUDO='$([ "$(id -u)" = 0 ] && echo "" || echo sudo)'

echo ">> sync knowledge + server to $VM"
ssh "$VM" "mkdir -p '$KNOW_DIR/docs' '$KNOW_DIR/skills' '$SRV_DIR'"
rsync -az --delete "$REPO_ROOT/docs/"   "$VM:$KNOW_DIR/docs/"
rsync -az --delete "$REPO_ROOT/skills/" "$VM:$KNOW_DIR/skills/"
rsync -az "$REPO_ROOT/CLAUDE.md" "$VM:$KNOW_DIR/CLAUDE.md"
rsync -az "$REPO_ROOT/mcp/firmware-knowledge/server.py" \
          "$REPO_ROOT/mcp/firmware-knowledge/pyproject.toml" \
          "$REPO_ROOT/mcp/firmware-knowledge/README.md" \
          "$VM:$SRV_DIR/"

echo ">> install deps + systemd + register upstream on $VM"
ssh "$VM" "PORT='$PORT' KNOW_DIR='$KNOW_DIR' SRV_DIR='$SRV_DIR' bash -s" <<'REMOTE'
set -euo pipefail
SUDO=""; [ "$(id -u)" = 0 ] || SUDO="sudo"
# Files synced over root SSH are root-owned; the service runs as mcp, so hand
# ownership back and run uv sync as mcp (uv + venv live in mcp's home).
$SUDO chown -R mcp:mcp "$KNOW_DIR" "$SRV_DIR"
sudo -u mcp bash -c "cd '$SRV_DIR' && /home/mcp/.local/bin/uv sync"

$SUDO tee /etc/systemd/system/mcp-firmware-knowledge.service >/dev/null <<UNIT
[Unit]
Description=Firmware Knowledge MCP Server
After=network.target

[Service]
Type=simple
User=mcp
Group=mcp
WorkingDirectory=$SRV_DIR
Environment="PATH=/home/mcp/.local/bin:/usr/local/bin:/usr/bin:/bin"
Environment="FW_KNOWLEDGE_ROOT=$KNOW_DIR"
ExecStart=/home/mcp/.local/bin/uv run python server.py streamable-http 127.0.0.1 $PORT
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
UNIT

$SUDO systemctl daemon-reload
$SUDO systemctl enable --now mcp-firmware-knowledge.service

python3 - "$PORT" <<'PY'
import json, pathlib, sys
port = sys.argv[1]
cfg = pathlib.Path("/home/mcp/mcp-gateway/configs/proxy-config.json")
d = json.loads(cfg.read_text())
servers = d.setdefault("servers", [])
servers[:] = [s for s in servers if s.get("name") != "firmware-knowledge"]
servers.append({
    "name": "firmware-knowledge",
    "url": f"http://127.0.0.1:{port}",
    "prefix": "fw",
    "enabled": True,
    "timeout": 30.0,
    "description": "RAK4630 firmware + LoRaWAN auto-provisioning knowledge",
})
cfg.write_text(json.dumps(d, indent=2) + "\n")
print("registered firmware-knowledge upstream")
PY

$SUDO systemctl restart mcp-proxy.service
sleep 2
echo "active states:"
systemctl is-active mcp-firmware-knowledge.service mcp-proxy.service
REMOTE
echo ">> done."
