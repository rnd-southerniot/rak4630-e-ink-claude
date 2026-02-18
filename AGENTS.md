# AGENTS.md

## Project Scope

Repository: `rak4630-e-ink`
Goal: Implement a WisBlock environmental node that reads VOC + BMP280 data, renders values on RAK14000 E-Ink, and uplinks via LoRaWAN OTAA to ChirpStack v4 (AS923-1), with FUOTA and rollback support.

## Locked Hardware Baseline

- Core module: `RAK3312` (ESP32-S3 + SX1262 LoRa)
- Base board: `RAK19007` (WisBlock Base Board 2nd Gen)
- Display: `RAK14000` (E-Ink, IO slot)
- VOC sensor: `RAK12047` (SGP40)
- Pressure sensor: external `BMP280` on I2C header

## Recommended Slot / Wiring Map

- Core: `RAK3312` in core socket
- IO slot: `RAK14000` (required)
- Sensor slot A: `RAK12047` (SGP40)
- External header: `BMP280` to 4-pin I2C header (`3V3`, `GND`, `SDA`, `SCL`)

Reasoning:
- `RAK14000` consumes IO slot.
- `RAK3312` already includes LoRa, so no extra LPWAN IO module is required.
- SGP40 and BMP280 both run on I2C without address conflict in normal configuration.

## Locked Deployment Profile

- Country: `Bangladesh`
- Region profile: `AS923-1`
- ChirpStack deployment: `Docker`
- Gateway: `RAK7266` with Semtech UDP packet forwarder
- Runtime cadence: 5 minutes
- Display policy: refresh only on value-change threshold
- Display orientation: landscape
- FUOTA cadence: emergency-only
- FUOTA maximum downtime: 30 minutes

## Engineering Rules

1. Bring-up must be incremental and reversible.
2. Validate each subsystem independently before integration.
3. Do not change multiple hardware variables in one step.
4. Record exact wiring, slot placement, and firmware commit hash for every test run.
5. Keep production keys and secrets out of source.

## Required Bring-up Sequence

1. LED heartbeat + serial monitor baseline
2. E-Ink-only communication test
3. I2C smoke precheck (Gate 2.1) + address verification (`SGP40`, `BMP280`)
4. I2C presence gate + sensor read + plausibility checks + filtering
5. LoRaWAN OTAA join (`AS923-1`) and payload uplink to ChirpStack v4
6. Integrated cycle: sensor read -> display render -> uplink
7. Refresh interval tuning based on monitor/debug evidence
8. FUOTA pilot with rollback validation

## Canonical Gate IDs

- `0`: `env`
- `1`: `heartbeat`
- `2`: `display_smoke`
- `2.1`: `i2c_smoke` (internal selector `CONFIG_APP_GATE=21`)
- `3`: `i2c_presence`
- `4`: `sensor_pipeline`
- `5`: `payload_v1`
- `6`: `lorawan_join_uplink`
- `7`: `reliability_buffer`
- `8`: `fuota_scaffold`
- `9`: `live_publish`

## Definition of Done (Per Milestone)

A milestone is complete only when:
- Build succeeds from clean environment.
- Flash and serial monitor logs show expected markers.
- Test evidence is captured in docs (`logs`, screenshots, or serial traces).
- Rollback path is documented and tested where applicable.

## Documentation Rules

- Keep all project docs under `docs/`.
- Maintain one source of truth for architecture (`docs/02-data-flow.md`).
- Keep `docs/05-information-needed.md` updated with resolved/open items.
- Cite official sources when making hardware or protocol decisions.
- Keep reusable gate execution examples in `examples/gates/` (including Gate 1 heartbeat example).
