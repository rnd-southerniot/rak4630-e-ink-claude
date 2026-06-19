# firmware-knowledge MCP server

An MCP server that serves this repo's RAK4630 firmware-development and LoRaWAN
auto-provisioning knowledge (docs, skills, gate evidence) as queryable tools, behind the
SIoT MCP gateway (`10.10.8.113:8000`). Future agents can develop firmware / provision nodes
by querying it instead of needing the local files.

## Tools

| Tool | Returns |
|------|---------|
| `list_docs()` | All knowledge docs (`path`, `title`) |
| `get_doc(path)` | Full markdown of a doc |
| `search(query, max_results=8)` | Matching `{path, line, snippet}` across all docs |
| `get_provisioning_guide()` | `docs/12-auto-provisioning-workflow.md` |
| `get_sop_guide()` | `docs/13-crm-sop-template-and-product-setup.md` |
| `get_gate_status()` | `docs/GATE_EXECUTION_LOG.md` |
| `get_pin_map()` | `docs/11-pin-mapping-rak4630-rak19007.md` |

## Accessing through the gateway

This gateway does **not** flatten upstream tools into the top-level `tools/list`; it exposes
**meta-tools**. Reach the knowledge server (registered as `firmware-knowledge`, prefix `fw`)
via:

- `discover_upstream_tools(server_name="firmware-knowledge")` → lists the tools above.
- `call_upstream_tool(server_name="firmware-knowledge", tool_name="get_provisioning_guide", arguments={})`
  → invokes a tool.

`list_upstream_servers()` confirms it's registered.

## Architecture

A FastMCP server serving `/mcp` over streamable-HTTP on `127.0.0.1:8002` — the same upstream
contract as the gateway's other servers. The proxy at `:8000` aggregates it under the `fw`
prefix. Knowledge is read from `FW_KNOWLEDGE_ROOT` (default
`/home/mcp/knowledge/rak4630-e-ink-claude`), a synced checkout of this repo on the VM.

## Deploy / update

Run from the repo root (syncs the latest docs/skills, (re)registers the upstream, restarts the
proxy). This is also how you **refresh** the knowledge after editing docs:

```bash
mcp/firmware-knowledge/install-on-vm.sh
```

Requires SSH access to the gateway VM (`ssh mcp-vm`) with privileges to write
`/etc/systemd/system` and restart services.

## Verify

```bash
# from a host that can reach the gateway
curl -s http://10.10.8.113:8000/health
# JSON-RPC tools/list against the gateway (Bearer token from the gateway api-key)
```

`fw:*` tools should appear and return content; `systemctl status mcp-firmware-knowledge`
should be active.
