# CLAUDE.md

> **Repo consolidated onto ESP-IDF / ESP32-S3 (P6, 2026-07-06).** The product is the **RAK3312
> (ESP32-S3 + SX1262)** built with **ESP-IDF v5.5.4** as the single canonical path — aligned with
> `careflow-modbus-node` and the central CRM/ChirpStack firmware-automation. The former
> PlatformIO/Arduino + nRF52840 (RAK4631) tree under `pio/` has been **removed**; recover it from the
> `pio-nrf52-archive` git tag if ever needed. New repo name: **`senseflow-eink-node`**.
> Migration plan: `~/.claude/plans/generic-jingling-owl.md`.

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP-IDF firmware for a WisBlock-class environmental node (**RAK3312**, ESP32-S3 + SX1262) that reads
BMP280/BME280 (and optionally SGP40 VOC) over I²C, renders values on a RAK14000 E-Ink display
(SSD1680), and uplinks via LoRaWAN OTAA to ChirpStack v4 (AS923-1 / Bangladesh). Telemetry uses the
ADR-006 device-profile model: a provisioned profile (in NVS) drives a generic sensor read + ADR-005
compact payload encode — the same "provision by data, one generic image" model as careflow.

## Build and Flash (ESP-IDF — the only path)

```bash
source ~/esp/esp-idf-v5.5.4/export.sh
idf.py -C firmware set-target esp32s3          # first time
idf.py -C firmware build
idf.py -C firmware -p /dev/cu.usbmodem1401 flash monitor
```

Configuration is via **Kconfig** (`firmware/sdkconfig.defaults` for pinned defaults; `idf.py
-C firmware menuconfig` to change). There is no PlatformIO and no `-D` build flags. Two config axes
matter most:

- **`CONFIG_APP_GATE=N`** — selects one bring-up gate (see below). Default gate path.
- **`CONFIG_APP_LORA_SMOKE=y`** — bypasses the gate runner and runs the **real** LoRaWAN field app:
  `lora_init` (SX1262/RadioLib) → OTAA join → load NVS device-profile → `profile_read` →
  `dp_encode_payload` (ADR-005) → `lora_send`. This is the production telemetry path. With it `n`,
  the gate runner uses a LoRaWAN **stub** (`lorawan_service.c`) for display/sensor bring-up only.

`sdkconfig` is gitignored; `sdkconfig.defaults` is the tracked source of truth.

## Running Tests

Host-native unit tests (pure C, no hardware, no ESP-IDF) build the ESP-IDF firmware's pure logic and
run under ctest — the same path CI uses:

```bash
tests/host/run_tests.sh          # cmake -S tests/host && ctest
```

Covers `device_profile` (ADR-006 serialize/deserialize + ADR-005 encode, proven on-hardware) and
`gate_id_map` (legacy gate-ID translation). Compiled with `-Wall -Wextra -Werror`.

## Gate-Driven Architecture

When `CONFIG_APP_LORA_SMOKE` is off, the firmware executes exactly one "gate" per flash cycle,
selected by `CONFIG_APP_GATE=N`. `app_gate_tick()` runs on a `CONFIG_APP_GATE_TICK_MS` (200 ms) tick;
on PASS the firmware halts and logs `result=PASS gate=<N>`. To advance, change the Kconfig value and
reflash.

### Gate IDs (canonical)

| ID | Name | What it validates |
|----|------|-------------------|
| 0 | `env` | Toolchain sanity (instant pass) |
| 1 | `heartbeat` | Heartbeat LED toggle |
| 2 | `display_smoke` | SPI bus + SSD1680 hello world render |
| 21 | `i2c_smoke` | I2C bus scan + device probe (Gate 2.1) |
| 3 | `i2c_presence` | SGP40/BMP280 address presence |
| 4 | `sensor_pipeline` | Sensor identity + sample read + plausibility |
| 5 | `payload_v1` | Payload encode against known test vector |
| 6 | `lorawan_join_uplink` | OTAA join + first uplink (stub backend) |
| 7 | `reliability_buffer` | Store-and-forward buffer flush (stub) |
| 8 | `fuota_scaffold` | FUOTA hook readiness (placeholder) |
| 9 | `live_publish` | Full cycle: sensor read → display render → uplink (stub backend) |

> For **real** end-to-end LoRaWAN telemetry (not the gate stub), build with `CONFIG_APP_LORA_SMOKE=y`
> — that is the path validated against dev ChirpStack (BME280 → `temp_C`/`pressure_hPa` decode).

### Per-gate device expectations

Gates 21, 3, 4, and 9 use `CONFIG_APP_GATE<N>_EXPECTED_DEVICES` (1=SGP40 only, 2=BMP280 only, 3=both)
to control which I2C devices must be present for PASS.

## Module Map (ESP-IDF)

Application code lives in `firmware/main/`; reusable pure logic in `firmware/components/`:

- **app_main.c** — entry point; branches on `CONFIG_APP_LORA_SMOKE` (real field app vs gate runner).
- **app_gate.c** — gate state machine, tick dispatch, pass/fail/halt logic.
- **gate_id_map.c** — legacy→canonical gate ID translation (pure C, host-tested).
- **app_payload.c** — legacy gate payload v1 encode + hex formatter (pure C).
- **components/device_profile/** — ADR-006 multi-bus (v2) profile core: `dp_serialize/deserialize`,
  `dp_encode_payload` (ADR-005), `dp_simulate` (pure C, host-tested).
- **profile_reader.c / profile_store.c** — read a sensor per the NVS profile; load/store the blob.
- **sensor_service.c** — SGP40/BMP280 identity + sample pipeline (BMP280 compensated read).
- **display_service.c / display_epd_ssd1680.c / display_rows.c** — refresh policy + SSD1680 transport.
- **lora.cpp / EspHalS3.h** — RadioLib SX1262 LoRaWAN (vendored S3 HAL); `lora.h` C API.
- **lorawan_service.c** — LoRaWAN **stub** used by the gate runner only.
- **provisioning.c / prov_console.cpp** — NVS provisioning console (`prov-*` commands).
- **buttons.c** — RAK14000 buttons.

## Hardware Pin Mapping (RAK3312 / ESP32-S3)

Source of truth: `firmware/main/gpio_remap.h` + the display/I²C Kconfig in `firmware/main/Kconfig.projbuild`.
Verified on-bench assignments:

- **I²C** (`CONFIG_APP_I2C_*`): SDA=GPIO9, SCL=GPIO40, 100 kHz; SGP40 @0x59, BMP280/BME280 @0x76.
- **Display SPI** (SSD1680, `CONFIG_APP_DISPLAY_PIN_*`): MOSI=11, SCLK=13, CS=12, DC=21, BUSY=42
  (active-**high** on RAK14000 → `CONFIG_APP_DISPLAY_BUSY_ACTIVE_HIGH=y`), PWR=14; RST unused.
- **LoRa SPI (SX1262)**: see `firmware/main/lora.cpp` / `gpio_remap.h` (SCK/MISO/MOSI + NSS/RST/BUSY/
  DIO1); AS923 with TCXO (1.8 V) and DIO2 as the RF switch.
- **Buttons**: S1=GPIO41, S2=GPIO38, S3=GPIO39 (active-low).
- Panel: SSD1680 250×122, landscape. Full tri-colour refresh ~15–20 s.

## LoRaWAN Credentials & Provisioning

Real LoRaWAN uses **RadioLib** (SX1262). OTAA creds and the device-profile live in **NVS** (namespace
`prov`), loaded at boot; the compiled `firmware/main/lora_credentials.h` (gitignored) is a fallback
only, and `lora_credentials.h.example` (all-zero placeholder) keeps a clean checkout building. DevEUI
is derived from the ESP32-S3 MAC (EUI-48→EUI-64 FF:FE) unless NVS overrides it.

Provision from `firmware/.env` (gitignored) over the USB-Serial-JTAG console:

```bash
# From the central hub (careflow repo), senseflow-aware:
python3 tools/provision_nvs.py --product senseflow -p /dev/cu.usbmodem1401
# prov-lorawan <deveui> <joineui> <appkey> ; prov-profile <hexblob> ; prov-show ; prov-done
```

Provisioning is **additive** (writes only the `prov` namespace) so LoRaWAN nonces/session survive.

## Engineering Rules

- Bring-up is incremental and reversible — one gate at a time; `CONFIG_APP_LORA_SMOKE=y` for the real
  field app.
- Validate each subsystem independently before integration.
- Do not change multiple hardware variables in one step.
- Record exact wiring, slot placement, and firmware commit hash for every test run.
- Keep production keys out of source (NVS/`.env`, never committed).
- A gate is done when: clean `idf.py -C firmware build` succeeds, serial monitor shows `result=PASS`,
  and test evidence is captured.

## Documentation

- Architecture source of truth: `docs/02-data-flow.md`
- Auto-provisioning workflow: `docs/12-auto-provisioning-workflow.md`
- CRM product/SOP-template setup: `docs/13-crm-sop-template-and-product-setup.md`
- Device profiles + CRM catalog: `device-profiles/README.md`
- Open items tracker: `docs/05-information-needed.md`
- Tools: `tools/provision-node.sh` (provisioning), `tools/serial_capture.py` (serial capture)

> **Note:** docs written for the retired PlatformIO/nRF52 tree may still reference `pio/`, `-DAPP_GATE`,
> or SX126x-Arduino; treat those as historical — the canonical path is ESP-IDF (`firmware/`) + Kconfig.
