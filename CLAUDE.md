# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP-IDF firmware for a WisBlock environmental node (RAK3312/ESP32-S3 core on RAK19007 base) that reads VOC (SGP40) + BMP280 sensor data, renders values on a RAK14000 E-Ink display (SSD1680), and uplinks via LoRaWAN OTAA to ChirpStack v4 (AS923-1 / Bangladesh). FUOTA support is emergency-only with mandatory rollback, max 30 min downtime.

## Build and Flash

Requires ESP-IDF v5.5 toolchain. All `idf.py` commands target the `firmware/` directory:

```bash
source $IDF_PATH/export.sh
idf.py -C firmware build
idf.py -C firmware -p /dev/cu.usbmodem1101 flash monitor
```

To change gate or other config interactively:

```bash
idf.py -C firmware menuconfig    # menu: "RAK4630 E-Ink Node"
```

Configuration defined in `firmware/main/Kconfig.projbuild`, defaults in `firmware/sdkconfig.defaults`.

## Running Tests

Host-side unit tests (no hardware needed) compile firmware C modules with the system C compiler:

```bash
tests/host/run_tests.sh
```

This builds and runs both `payload_encode_test` and `gate_id_map_test` under `tests/host/.build/`. To run a single test after building:

```bash
tests/host/.build/payload_encode_test
tests/host/.build/gate_id_map_test
```

Tests use deterministic test vectors and print `PASS` on success or return nonzero on failure. Compiled with `-Wall -Wextra -Werror`.

## Gate-Driven Architecture

The firmware executes exactly one "gate" per flash cycle, controlled by `CONFIG_APP_GATE` in sdkconfig. The main loop (`app_main.c`) initializes `app_gate_ctx_t` and calls `app_gate_tick()` on a 200ms tick. On PASS, the firmware halts and logs `result=PASS gate=<N>`. To advance, change `CONFIG_APP_GATE` and reflash.

### Gate IDs (canonical)

| ID | Name | What it validates |
|----|------|-------------------|
| 0 | `env` | Toolchain sanity (instant pass) |
| 1 | `heartbeat` | LED toggle on GPIO 46 |
| 2 | `display_smoke` | SPI bus + SSD1680 hello world render |
| 21 | `i2c_smoke` | I2C bus scan + device probe (Gate 2.1) |
| 3 | `i2c_presence` | SGP40/BMP280 address presence |
| 4 | `sensor_pipeline` | Sensor identity + sample read + plausibility |
| 5 | `payload_v1` | Payload encode against known test vector |
| 6 | `lorawan_join_uplink` | OTAA join + first uplink |
| 7 | `reliability_buffer` | Store-and-forward buffer flush after join |
| 8 | `fuota_scaffold` | FUOTA hook readiness (placeholder) |
| 9 | `live_publish` | Full cycle: sensor read -> display render -> uplink |

A legacy ID scheme (`CONFIG_APP_GATE_ID_SCHEME_LEGACY`) maps 0..8 to canonical IDs via `gate_id_map.c`.

### Selecting and running a gate

Helper scripts in `examples/gates/`:
- `set_gate_new.sh <id>` — set canonical gate in sdkconfig (accepts `2.1` for i2c_smoke)
- `set_gate_legacy.sh <id>` — set legacy gate
- `run_gate.sh <id>` — set + build + flash + monitor in one step
- Per-gate scripts: `run_gate_1_heartbeat.sh`, `run_gate_2_display.sh`, `run_gate_2_1_i2c.sh`, `run_gate_3_i2c_presence.sh`, `run_gate_4_sensor_pipeline.sh`, `run_gate_6_lorawan_join_uplink.sh`, `run_gate_7_reliability_buffer.sh`, `run_gate_8_fuota_scaffold.sh`, `run_gate_9_live_publish.sh`

### Per-gate device expectations

Gates 2.1, 3, 4, and 9 use `CONFIG_APP_GATE<N>_EXPECTED_DEVICES` (1=SGP40 only, 2=BMP280 only, 3=both) to control which I2C devices must be present for PASS.

## Firmware Module Map

All source lives in `firmware/main/`. The gate runner (`app_gate.c`) is the central orchestrator; services are initialized conditionally based on which gate is selected:

- **app_gate** — gate state machine, tick dispatch, pass/fail/halt logic
- **gate_id_map** — legacy-to-canonical gate ID translation (pure function, host-testable)
- **app_payload** — payload v1 binary encode (12 bytes big-endian) + hex formatter (pure, host-testable)
- **app_types** — shared `sensor_sample_t` struct
- **heartbeat** — FreeRTOS task toggling LED GPIO
- **i2c_bus** — I2C master init, scan, probe, read/write primitives
- **sensor_service** — SGP40/BMP280 identity check + sample pipeline
- **display_service** — threshold-based refresh policy + render orchestration
- **display_epd_ssd1680** — low-level SPI transport for SSD1680 (cmd/data, busy-wait, power control)
- **display_font_5x7** — bitmap font for E-Ink text rendering
- **lorawan_service** — join state machine, uplink send, metrics tracking (currently stub-based)

## LoRaWAN Credentials

Gates 6, 7, and 9 require OTAA credentials. Copy `firmware/.env.example` to `firmware/.env` and fill `DEVEUI` and `APPKEY` (uppercase hex, no `0x` prefix). `JOINEUI` defaults to all-zeros. The `.env` file is gitignored.

## Key Configuration Values (sdkconfig.defaults)

- I2C: SDA=GPIO9, SCL=GPIO40, 100kHz, SGP40 at 0x59, BMP280 at 0x76
- Display SPI: MOSI=11, SCLK=13, CS=12, DC=21, BUSY=42, PWR=14 (active-high input-pullup)
- Panel: SSD1680 250x122, landscape orientation
- Tick: 200ms gate runner period
- Production cadence: 300s sample + uplink, 30s min display refresh
- Pin mapping source of truth: `docs/11-pin-mapping-rak3312-rak19007.md`

## Engineering Rules

- Bring-up is incremental and reversible — one gate at a time
- Validate each subsystem independently before integration
- Do not change multiple hardware variables in one step
- Record exact wiring, slot placement, and firmware commit hash for every test run
- Keep production keys out of source (use `firmware/.env`)
- A gate is done when: build succeeds from clean, serial monitor shows expected `result=PASS` markers, and test evidence is captured in `docs/`

## Documentation Structure

- Architecture source of truth: `docs/02-data-flow.md`
- Open items tracker: `docs/05-information-needed.md`
- Gate execution examples: `examples/gates/`
- Troubleshooting skill: `skills/rak3312-gate-troubleshooting/SKILL.md`
