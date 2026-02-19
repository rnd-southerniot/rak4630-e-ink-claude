# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Arduino/PlatformIO firmware for a WisBlock environmental node (RAK4630/nRF52840 core on RAK19007 base) that reads VOC (SGP40) + BMP280 sensor data, renders values on a RAK14000 E-Ink display (SSD1680), and uplinks via LoRaWAN OTAA to ChirpStack v4 (AS923-1 / Bangladesh).

Ported from ESP-IDF (ESP32-S3) to Arduino/PlatformIO targeting the nRF52840. The gate-driven architecture is preserved 1:1.

## Build and Flash (PlatformIO — Primary)

All PlatformIO commands run from the `pio/` directory:

```bash
cd pio/
~/.platformio/penv/bin/pio run                              # build
~/.platformio/penv/bin/pio run -t upload -t monitor          # flash + serial monitor
~/.platformio/penv/bin/pio run -t clean && pio run           # clean build
```

Configuration is via `-D` build flags in `pio/platformio.ini` (no Kconfig/menuconfig). Change `-DAPP_GATE=N` to select the gate.

## Build and Flash (ESP-IDF — Legacy)

The original ESP-IDF firmware lives in `firmware/`. It targets ESP32-S3 and does **not** run on the RAK4630/nRF52840. Kept for reference only.

```bash
source $IDF_PATH/export.sh
idf.py -C firmware build
idf.py -C firmware -p /dev/cu.usbmodem1101 flash monitor
```

## Running Tests

Host-side unit tests (no hardware needed) compile pure C modules with the system compiler:

```bash
tests/host/run_tests.sh
```

Runs `payload_encode_test` and `gate_id_map_test` under `tests/host/.build/`. Tests print `PASS` on success, return nonzero on failure. Compiled with `-Wall -Wextra -Werror`.

## Gate-Driven Architecture

The firmware executes exactly one "gate" per flash cycle, controlled by `-DAPP_GATE=N` in `pio/platformio.ini`. The main loop calls `app_gate_tick()` on a 200ms tick. On PASS, the firmware halts and logs `result=PASS gate=<N>`. To advance, change `APP_GATE` and reflash.

### Gate IDs (canonical)

| ID | Name | What it validates |
|----|------|-------------------|
| 0 | `env` | Toolchain sanity (instant pass) |
| 1 | `heartbeat` | LED toggle on P1.03 (Green) |
| 2 | `display_smoke` | SPI bus + SSD1680 hello world render |
| 21 | `i2c_smoke` | I2C bus scan + device probe (Gate 2.1) |
| 3 | `i2c_presence` | SGP40/BMP280 address presence |
| 4 | `sensor_pipeline` | Sensor identity + sample read + plausibility |
| 5 | `payload_v1` | Payload encode against known test vector |
| 6 | `lorawan_join_uplink` | OTAA join + first uplink (stub) |
| 7 | `reliability_buffer` | Store-and-forward buffer flush (stub) |
| 8 | `fuota_scaffold` | FUOTA hook readiness (placeholder) |
| 9 | `live_publish` | Full cycle: sensor read -> display render -> uplink |

### Per-gate device expectations

Gates 21, 3, 4, and 9 use `APP_GATE<N>_EXPECTED_DEVICES` (1=SGP40 only, 2=BMP280 only, 3=both) to control which I2C devices must be present for PASS.

## PlatformIO Module Map

All source lives in `pio/src/` with headers in `pio/include/`:

- **app_gate.cpp** — gate state machine, tick dispatch, pass/fail/halt logic
- **gate_id_map.c** — legacy-to-canonical gate ID translation (pure C, host-testable)
- **app_payload.c** — payload v1 binary encode (12 bytes big-endian) + hex formatter (pure C, host-testable)
- **app_types.h** — shared `sensor_sample_t` struct
- **heartbeat.cpp** — cooperative millis()-based LED toggle
- **i2c_bus.cpp** — Arduino Wire-based I2C init, scan, probe, read/write
- **sensor_service.cpp** — SGP40/BMP280 identity check + sample pipeline
- **display_service.cpp** — threshold-based refresh policy + render orchestration
- **display_epd_ssd1680.cpp** — Arduino SPI transport for SSD1680
- **display_font_5x7.h** — bitmap font for E-Ink text rendering (header-only)
- **lorawan_service.cpp** — join state machine stub (SX126x-Arduino integration TODO)
- **sensirion_gas_index_algorithm.c** — Sensirion BSD-licensed VOC algorithm (pure C)
- **main.cpp** — Arduino setup()/loop() entry point
- **compat.h** — ESP-IDF compatibility shim (esp_err_t, ESP_LOG macros, error checks)
- **board_pins.h** — RAK4630 pin aliases referencing variant.h

## Hardware Pin Mapping (RAK4630 on RAK19007)

Pin mapping source of truth: `pio/variants/WisCore_RAK4631_Board/variant.h`
Pin aliases: `pio/include/board_pins.h`

Key assignments:
- I2C: SDA=P0.13, SCL=P0.14, 100kHz, SGP40 at 0x59, BMP280 at 0x76
- Display SPI (IO slot): MOSI=P0.30, SCK=P0.03, CS=P0.26, DC=P0.17, BUSY=P0.04, PWR=P1.02
- LoRa SPI (internal): NSS=P1.10, SCLK=P1.11, MOSI=P1.12, MISO=P1.13 (managed by SX126x-Arduino library)
- Panel: SSD1680 250x122, landscape orientation
- LEDs: Green=P1.03, Blue=P1.04 (active HIGH)
- Tick: 200ms gate runner period

## Key Build Flags (pio/platformio.ini)

| Flag | Default | Description |
|------|---------|-------------|
| `APP_GATE` | 0 | Which gate to run |
| `APP_GATE_TICK_MS` | 200 | Gate runner tick period (ms) |
| `APP_I2C_FREQ_HZ` | 100000 | I2C bus frequency |
| `APP_DISPLAY_SPI_MHZ` | 4 | E-Ink SPI clock (MHz) |
| `APP_SGP40_ADDR` | 0x59 | SGP40 I2C address |
| `APP_BMP280_ADDR` | 0x76 | BMP280 I2C address |
| `APP_DISPLAY_LANDSCAPE` | 1 | Landscape orientation |
| `APP_DISPLAY_MIN_REFRESH_SEC` | 30 | Min seconds between display refreshes |
| `APP_LORAWAN_BACKEND_ACTIVE` | 1 | Enable LoRaWAN backend |
| `APP_BATTERY_ADC_ENABLED` | 1 | Enable battery voltage ADC |

## LoRaWAN Credentials

Gates 6, 7, and 9 require OTAA credentials. The LoRaWAN service is currently a **stub** in the PlatformIO port. Real SX126x-Arduino OTAA integration needs `lora_rak4630_init()` + `lmh_init()` with `LORAMAC_REGION_AS923`.

Keep credentials in `firmware/.env` (gitignored): `DEVEUI`, `APPKEY` (uppercase hex, no `0x`). `JOINEUI` defaults to all-zeros.

## Engineering Rules

- Bring-up is incremental and reversible — one gate at a time
- Validate each subsystem independently before integration
- Do not change multiple hardware variables in one step
- Record exact wiring, slot placement, and firmware commit hash for every test run
- Keep production keys out of source
- A gate is done when: build succeeds from clean, serial monitor shows `result=PASS` markers, and test evidence is captured

## Skills

- `skills/rak4630-gate-troubleshooting/` — Troubleshoot gate failures
- `skills/rak4630-pio-build/` — Build, flash, monitor workflow
- `skills/rak4630-pin-reference/` — Quick pin reference card
- `skills/rak3312-gate-troubleshooting/` — Legacy ESP-IDF troubleshooting (archived)

## Documentation

- Architecture source of truth: `docs/02-data-flow.md`
- Open items tracker: `docs/05-information-needed.md`
- Legacy gate scripts: `examples/gates/`
