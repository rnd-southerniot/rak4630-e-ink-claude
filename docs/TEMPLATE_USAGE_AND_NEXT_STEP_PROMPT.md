# Template Usage And Next-Step Prompt

This guide explains how to use the two CTO-grade Codex templates in this repo.

## Templates

1. Current project model template:
- `/Users/arif/rak4630-e-ink/docs/CTO_CODEX_MULTI_AGENT_SUPER_PROMPT_TEMPLATE.md`

2. Architecture-model template (`core/hal/drivers/config/app/main.c`):
- `/Users/arif/rak4630-e-ink/docs/CTO_CODEX_MULTI_AGENT_SUPER_PROMPT_TEMPLATE_ARCH_MODEL.md`

## When To Use Which

1. Use the current project model template when:
- You are continuing gate execution, debugging, display/sensor/LoRaWAN integration, and doc/evidence updates in the existing structure.

2. Use the architecture-model template when:
- You want to enforce or migrate to the strict architecture:
  - `core` immutable business logic
  - `hal` interfaces
  - `drivers` low-level peripherals
  - `config` centralized profiles/flags
  - `app` orchestration

## How To Use In Codex (Step-by-Step)

1. Open the template file you want to use.
2. Copy the `Copy/Paste Super Prompt` block from that file.
3. Replace placeholders:
- `Current task: <REPLACE>`
- `Target gate: <REPLACE>`
- `Device mode (if needed): <1|2|3>`
4. Paste into a new Codex message.
5. Keep the gate handshake policy:
- Do one gate only.
- Require PASS marker.
- Stop-after-pass and update docs/checklist/log.

## Run On Codex CLI

### Option A: Interactive Session (recommended for iterative bring-up)

```bash
cd /Users/arif/rak4630-e-ink
codex -C /Users/arif/rak4630-e-ink
```

Then paste the selected template prompt block and continue gate-by-gate.

### Option B: Non-Interactive Run From STDIN (`codex exec`)

```bash
cat <<'PROMPT' | codex exec -C /Users/arif/rak4630-e-ink --sandbox workspace-write -
Use /Users/arif/rak4630-e-ink/docs/CTO_CODEX_MULTI_AGENT_SUPER_PROMPT_TEMPLATE.md as the operating contract.

Current task:
- Run Gate 9 in SGP40-only mode, capture PASS evidence, and update docs/checklist/log.

Target gate:
- 9

Device mode:
- 1
PROMPT
```

### Option C: Run From Prompt File

```bash
codex exec -C /Users/arif/rak4630-e-ink --sandbox workspace-write - < /path/to/prompt.txt
```

Optional:
- add `-o /tmp/codex_last_message.txt` to save the final assistant message.

## Next-Step Prompt (Recommended)

Use this prompt to continue delivery immediately:

```text
Use /Users/arif/rak4630-e-ink/docs/CTO_CODEX_MULTI_AGENT_SUPER_PROMPT_TEMPLATE.md as the operating contract.

Current task:
- Run a full quality pass on Gate 9 (SGP40-only mode), verify deterministic logs and display readability, then update all evidence docs.
- After that, prepare a controlled transition plan for real LoRaWAN radio path integration (replace stub path) without breaking existing gate contracts.

Target gate:
- 9

Device mode:
- 1

Mandatory outputs:
1) concise execution plan
2) exact commands run
3) PASS/FAIL evidence lines
4) exact files updated
5) residual risks + next gate/next milestone command
```

### Run The Recommended Next-Step Prompt In Codex CLI

```bash
cat <<'PROMPT' | codex exec -C /Users/arif/rak4630-e-ink --sandbox workspace-write -
Use /Users/arif/rak4630-e-ink/docs/CTO_CODEX_MULTI_AGENT_SUPER_PROMPT_TEMPLATE.md as the operating contract.

Current task:
- Run a full quality pass on Gate 9 (SGP40-only mode), verify deterministic logs and display readability, then update all evidence docs.
- After that, prepare a controlled transition plan for real LoRaWAN radio path integration (replace stub path) without breaking existing gate contracts.

Target gate:
- 9

Device mode:
- 1

Mandatory outputs:
1) concise execution plan
2) exact commands run
3) PASS/FAIL evidence lines
4) exact files updated
5) residual risks + next gate/next milestone command
PROMPT
```

## Next-Step Prompt (Architecture Migration)

Use this when you want to start structural refactor with minimal behavior change:

```text
Use /Users/arif/rak4630-e-ink/docs/CTO_CODEX_MULTI_AGENT_SUPER_PROMPT_TEMPLATE_ARCH_MODEL.md as the operating contract.

Current task:
- Propose and execute Phase 1 architecture migration to:
  firmware/core, firmware/hal/{mcu,sensors,comm}, firmware/drivers, firmware/config, firmware/app, firmware/main.c
- Keep runtime behavior identical and keep all gate IDs/markers unchanged.
- Do not modify firmware/core business logic unless explicitly approved.

Target gate:
- 0..2 smoke validation after refactor

Mandatory outputs:
1) migration map (old file -> new file)
2) architecture compliance report
3) build + flash/monitor evidence for selected smoke gates
4) updated docs and rollback steps
```

### Run The Architecture-Migration Prompt In Codex CLI

```bash
cat <<'PROMPT' | codex exec -C /Users/arif/rak4630-e-ink --sandbox workspace-write -
Use /Users/arif/rak4630-e-ink/docs/CTO_CODEX_MULTI_AGENT_SUPER_PROMPT_TEMPLATE_ARCH_MODEL.md as the operating contract.

Current task:
- Propose and execute Phase 1 architecture migration to:
  firmware/core, firmware/hal/{mcu,sensors,comm}, firmware/drivers, firmware/config, firmware/app, firmware/main.c
- Keep runtime behavior identical and keep all gate IDs/markers unchanged.
- Do not modify firmware/core business logic unless explicitly approved.

Target gate:
- 0..2 smoke validation after refactor

Mandatory outputs:
1) migration map (old file -> new file)
2) architecture compliance report
3) build + flash/monitor evidence for selected smoke gates
4) updated docs and rollback steps
PROMPT
```
