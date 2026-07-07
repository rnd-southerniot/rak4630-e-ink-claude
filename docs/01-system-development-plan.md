# System Development Plan (Arduino/PlatformIO + RAK4630 / nRF52840 + RAK19007 + RAK14000)

> **⚠ Historical (pre-P6 / PlatformIO-nRF52).** This doc predates the ESP-IDF/ESP32-S3 consolidation and references the retired `pio/` tree. Canonical instructions now live in [CLAUDE.md](../CLAUDE.md) (ESP-IDF `firmware/` + Kconfig).

> Note: The original ESP-IDF / RAK3312 (ESP32-S3) version of this plan is legacy and superseded. The firmware was ported to Arduino/PlatformIO targeting the RAK4630 (nRF52840) in Feb 2026; the ESP-IDF tree in `firmware/` is retained for reference only. The gate model and milestone plan below are preserved 1:1.

## 1) Objective

Build an embedded node that:
- Reads VOC (`SGP40`) and pressure/temperature (`BMP280`) data
- Displays processed values on `RAK14000` E-Ink
- Sends uplinks to ChirpStack v4 over LoRaWAN OTAA (`AS923-1`)
- Supports FUOTA with rollback protection

## 2) Locked Design Decisions

- Framework: `Arduino` via `PlatformIO` (PlatformIO IDE / VSCode extension)
- Core module: `RAK4630` (nRF52840 + SX1262)
- Base board: `RAK19007`
- Display: `RAK14000`
- VOC sensor: `RAK12047` (`SGP40`)
- Pressure sensor: external `BMP280` via I2C header (`3.3V` native)
- Network server: `ChirpStack v4`
- ChirpStack deployment: `Docker`
- Gateway: `RAK7266` with Semtech UDP packet forwarder
- Region: `AS923-1`
- Deployment country: `Bangladesh`
- OTAA credentials: per-device
- Runtime cadence: 5-minute sampling and 5-minute uplink
- Display refresh policy: redraw only on value-change threshold
- Display orientation: landscape
- FUOTA cadence: emergency-only
- FUOTA maximum downtime: 30 minutes

## 3) Recommended Physical Mapping

- Core socket: `RAK4630`
- IO slot: `RAK14000`
- Slot A: `RAK12047`
- 4-pin I2C header: external `BMP280`

Rationale:
- `RAK14000` needs the IO slot.
- LoRa is already integrated in `RAK4630`, avoiding IO-slot contention.

## 4) Canonical Gate Model

Bring-up and production validation follow stop-after-pass gates:

- Gate `0`: `env`
- Gate `1`: `heartbeat`
- Gate `2`: `display_smoke` (RAK14000, SPI)
- Gate `2.1`: `i2c_smoke` (internal selector `-DAPP_GATE=21`)
- Gate `3`: `i2c_presence`
- Gate `4`: `sensor_pipeline`
- Gate `5`: `payload_v1`
- Gate `6`: `lorawan_join_uplink`
- Gate `7`: `reliability_buffer`
- Gate `8`: `fuota_scaffold`
- Gate `9`: `live_publish`

## 5) Milestone Plan

## M0 - Hardware and Region Lock (Completed)

- Confirm RAK4630 (nRF52840) hardware path
- Confirm sensor choices (`SGP40` + external `BMP280`)
- Confirm LoRa region (`AS923-1`) and ChirpStack major version (v4)

## M1 - Project Skeleton and Build Baseline

- Create firmware structure (`pio/src`, `pio/include`, `pio/scripts`)
- Configure PlatformIO IDE / VSCode extension and project tasks
- Add boot banner with firmware version, git hash, board profile

Exit criteria:
- Clean build and flash from fresh clone
- Monitor output captured

## M2 - Board Sanity (LED Heartbeat)

- Implement non-blocking heartbeat task
- Verify stable runtime and no watchdog reset

Exit criteria:
- 10+ minute stability run

## M3 - E-Ink-Only Bring-up

- Initialize RAK14000 bus and panel
- Render static test frame + incrementing counter
- Measure full and partial refresh timings

Exit criteria:
- Deterministic updates with no init/bus failures

## M4 - I2C Discovery and Sensor Validation

- Add I2C scanner at boot
- Validate SGP40 and BMP280 presence
- Read and log raw sensor values with plausibility checks

Exit criteria:
- Stable address detection over 5 reboots
- Sensor read success rate >= 99% over 500 samples

## M5 - Data Processing and Display Model

- Apply filtering (EWMA recommended)
- Convert data to engineering units
- Define landscape display layout and threshold-based refresh policy

Exit criteria:
- No display ghosting regression at target cadence
- Refresh occurs only when value-change threshold is crossed
- Layout is validated in landscape mode on target panel

## M6 - LoRaWAN OTAA + ChirpStack v4

- Provision per-device `DevEUI/JoinEUI/AppKey`
- Configure `AS923-1` channel plan
- Implement join, uplink, retry/backoff state machine

Exit criteria:
- OTAA join accepted by ChirpStack v4
- Decoded payload matches device-side values

## M7 - Integrated Runtime and Tuning

- End-to-end loop: read -> process -> display -> uplink
- Maintain 5-minute cycle
- Add fault paths (sensor missing, join fail, display timeout)

Exit criteria:
- 24h soak test with no fatal resets

## M8 - FUOTA Pilot + Rollback

- Integrate FUOTA workflow with ChirpStack v4
- Validate update campaign in lab
- Enforce rollback on failed post-update health checks

Exit criteria:
- Successful pilot update and verified rollback path

## 6) VSCode Execution Model (PlatformIO)

- Install the `PlatformIO IDE` extension for VSCode
- Target board/variant: RAK4630 (nRF52840), `WisCore_RAK4631_Board`; one-time board/variant install is described in the `pio/platformio.ini` header
- Standard flow (from the `pio/` directory):
  - `~/.platformio/penv/bin/pio run` (build)
  - `~/.platformio/penv/bin/pio run -t upload -t monitor` (flash + monitor, baud `115200`)
  - `~/.platformio/penv/bin/pio run -t clean && ~/.platformio/penv/bin/pio run` (clean build)

## 7) Risks and Mitigations

- Pin-map mismatch risk on new core: lock pin table in source and docs
- E-Ink refresh/power tradeoff: benchmark and tune threshold-based redraw
- AS923-1 misconfiguration: validate region in device + gateway + ChirpStack profile
- FUOTA failure risk: require rollback and staged rollout

## 8) Deliverables

- Arduino/PlatformIO firmware source tree (`pio/`)
- Wiring and slot map
- Bring-up and commissioning checklist
- ChirpStack v4 integration notes
- FUOTA rollback runbook
