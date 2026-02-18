# rak4630-e-ink

Gate-driven ESP-IDF firmware for a WisBlock environmental node with RAK14000 E-Ink display and LoRaWAN uplink.

## Locked Decisions

- Platform: ESP-IDF
- Core: `RAK3312`
- Base: `RAK19007`
- Display: `RAK14000` (SSD1680)
- Sensors: `SGP40` (`RAK12047`) + external `BMP280`
- Region: `AS923-1`
- Country: `Bangladesh`
- Gateway: `RAK7266` (UDP packet forwarder)
- Network server: `ChirpStack v4` (Docker)
- Cadence: 5 minutes
- Display: threshold refresh, landscape
- FUOTA: emergency-only, rollback required, max downtime 30 min

## Primary Docs

- Bring-up plan: `/Users/arif/rak4630-e-ink/docs/BRINGUP_PLAN.md`
- Gate handshake: `/Users/arif/rak4630-e-ink/docs/GATE_TRANSITION_HANDSHAKE.md`
- Gate run instructions: `/Users/arif/rak4630-e-ink/docs/GATE_RUN_INSTRUCTIONS.md`
- User instructions (CLI + IDE): `/Users/arif/rak4630-e-ink/docs/USERINSTRUCTION_GATES.md`
- Gate execution log: `/Users/arif/rak4630-e-ink/docs/GATE_EXECUTION_LOG.md`
- Canonical checklist: `/Users/arif/rak4630-e-ink/docs/CHECKLIST_GATES_0_TO_9.md`
- Pin mapping source of truth: `/Users/arif/rak4630-e-ink/docs/11-pin-mapping-rak3312-rak19007.md`
- CTO multi-agent template: `/Users/arif/rak4630-e-ink/docs/CTO_CODEX_MULTI_AGENT_SUPER_PROMPT_TEMPLATE.md`
- CTO architecture-model template: `/Users/arif/rak4630-e-ink/docs/CTO_CODEX_MULTI_AGENT_SUPER_PROMPT_TEMPLATE_ARCH_MODEL.md`
- Template usage + next-step prompt: `/Users/arif/rak4630-e-ink/docs/TEMPLATE_USAGE_AND_NEXT_STEP_PROMPT.md`

## Gate Notes

- Gate 1: `heartbeat` (LED sanity gate)
- Gate 2: `display_smoke` (E-Ink/SPI)
- Gate 2.1: `i2c_smoke` (I2C-only precheck, internal ID `21`)

## Reusable Gate Examples

- `/Users/arif/rak4630-e-ink/examples/gates/README.md`
- `/Users/arif/rak4630-e-ink/examples/gates/GATE_1_HEARTBEAT_EXAMPLE.md`
- `/Users/arif/rak4630-e-ink/examples/gates/GATE_2_DISPLAY_EXAMPLE.md`
- `/Users/arif/rak4630-e-ink/examples/gates/GATE_2_1_I2C_EXAMPLE.md`
- `/Users/arif/rak4630-e-ink/examples/gates/run_gate_1_heartbeat.sh`

## Firmware Entry

- `/Users/arif/rak4630-e-ink/firmware/README.md`
