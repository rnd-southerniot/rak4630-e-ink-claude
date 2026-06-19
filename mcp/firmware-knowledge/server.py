#!/usr/bin/env python3
"""
firmware-knowledge MCP server.

Serves RAK4630 firmware-development + LoRaWAN auto-provisioning knowledge
(the repo's docs/, skills/, gate evidence, CLAUDE.md) as queryable MCP tools so
agents working through the SIoT MCP gateway can develop firmware and provision
nodes without local file access.

Upstream contract (matches the gateway's other servers): a FastMCP server
serving /mcp over streamable-HTTP.
  Run: uv run python server.py streamable-http 0.0.0.0 8002

Knowledge root is a synced checkout of the rak4630-e-ink-claude repo on the VM
(see install-on-vm.sh). Override with FW_KNOWLEDGE_ROOT.
"""
import os
import sys
from pathlib import Path

from mcp.server.fastmcp import FastMCP

transport_mode = sys.argv[1] if len(sys.argv) > 1 else "streamable-http"
server_host = sys.argv[2] if len(sys.argv) > 2 else "0.0.0.0"
server_port = int(sys.argv[3]) if len(sys.argv) > 3 else 8002

ROOT = Path(os.environ.get("FW_KNOWLEDGE_ROOT", "/home/mcp/knowledge/rak4630-e-ink-claude")).resolve()

DOC_DIRS = ["docs", "skills"]
EXTRA_FILES = ["CLAUDE.md", "README.md"]

mcp = FastMCP("firmware-knowledge", host=server_host, port=server_port)


def _iter_docs():
    for d in DOC_DIRS:
        base = ROOT / d
        if base.is_dir():
            for p in sorted(base.rglob("*.md")):
                yield p
    for f in EXTRA_FILES:
        p = ROOT / f
        if p.is_file():
            yield p


def _rel(p: Path) -> str:
    try:
        return str(p.relative_to(ROOT))
    except ValueError:
        return str(p)


def _title(p: Path) -> str:
    try:
        for line in p.read_text(errors="replace").splitlines():
            if line.startswith("# "):
                return line[2:].strip()
    except Exception:
        pass
    return ""


@mcp.tool()
def list_docs() -> list[dict]:
    """List available firmware/provisioning knowledge documents (path + title)."""
    return [{"path": _rel(p), "title": _title(p)} for p in _iter_docs()]


@mcp.tool()
def get_doc(path: str) -> str:
    """Return the full markdown of a knowledge doc by its path (from list_docs())."""
    p = (ROOT / path).resolve()
    if p != ROOT and ROOT not in p.parents:
        return "error: path outside knowledge root"
    if not p.is_file():
        return f"error: not found: {path}"
    return p.read_text(errors="replace")


@mcp.tool()
def search(query: str, max_results: int = 8) -> list[dict]:
    """Case-insensitive substring search across all knowledge docs; returns {path,line,snippet}."""
    q = query.lower()
    hits: list[dict] = []
    for p in _iter_docs():
        try:
            lines = p.read_text(errors="replace").splitlines()
        except Exception:
            continue
        for i, line in enumerate(lines):
            if q in line.lower():
                snippet = "\n".join(lines[max(0, i - 1): i + 2])
                hits.append({"path": _rel(p), "line": i + 1, "snippet": snippet})
                if len(hits) >= max_results:
                    return hits
    return hits


@mcp.tool()
def get_provisioning_guide() -> str:
    """The end-to-end auto-provisioning workflow (CRM -> ChirpStack -> firmware/.env)."""
    return get_doc("docs/12-auto-provisioning-workflow.md")


@mcp.tool()
def get_sop_guide() -> str:
    """How to create the CRM LoRaWAN product + required SOP template prerequisites."""
    return get_doc("docs/13-crm-sop-template-and-product-setup.md")


@mcp.tool()
def get_gate_status() -> str:
    """The latest RAK4630 gate execution evidence log (what passes, on what hardware)."""
    return get_doc("docs/GATE_EXECUTION_LOG.md")


@mcp.tool()
def get_pin_map() -> str:
    """The RAK4630/RAK19007 pin map + WisBlock sensor-slot reference."""
    return get_doc("docs/11-pin-mapping-rak4630-rak19007.md")


if __name__ == "__main__":
    print(f"firmware-knowledge MCP on {server_host}:{server_port} ({transport_mode}); root={ROOT}", flush=True)
    mcp.run(transport="stdio" if transport_mode == "stdio" else "streamable-http")
