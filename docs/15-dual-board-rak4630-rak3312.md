# Dual-Board Firmware: RAK4630 (nRF52840) + RAK3312 (ESP32-S3)

One codebase builds for two WisBlock cores on the same RAK19007 base. Both are validated on
hardware (all 11 gates pass on each). ~90% of the firmware is board-agnostic.

## Build / flash

```bash
cd pio/
~/.platformio/penv/bin/pio run -e rak4631               # RAK4630 (nRF52840 + SX1262)
~/.platformio/penv/bin/pio run -e rak3312               # RAK3312 (ESP32-S3 + SX1262)
~/.platformio/penv/bin/pio run -e rak3312 -t upload -t monitor
```

Pick the gate with `-DAPP_GATE` (shared `[common]` in `platformio.ini`, edited by
`examples/gates/set_gate_new.sh`). `examples/gates/run_gate.sh <port> <env>` takes the env
(default `rak4631`).

## What's shared vs board-specific

| Layer | Files | Shared? |
|-------|-------|---------|
| Gate framework, gate catalog | `gate_framework.*`, `app_gate.*` | ✅ |
| Sensor registry + drivers | `sensor_registry.*`, `drivers/*`, `sensor_service.cpp` | ✅ |
| Payload, display logic, fonts | `app_payload.c`, `display_*` | ✅ |
| I2C bus, SPI/GPIO HAL | `i2c_bus.cpp`, `hal_*` | ✅ (small ESP32 bus-pin guards) |
| Pin map | `board_pins.h` → `board_pins_<board>.h` | ❌ per board |
| SX1262 radio + battery ADC | `board.h`, `src/board/<board>/` | ❌ per board |
| PlatformIO env | `platformio.ini` `[env:rak4631]` / `[env:rak3312]` | ❌ per board |

Board selection: each env defines `-DBOARD_RAK4631` / `-DBOARD_RAK3312` and a `build_src_filter`
that compiles only its `src/board/<board>/`. The board package exposes `board_radio_init()` and
`board_read_battery_volts()`; the shared `lorawan_service` / `sensor_service` call those.

## RAK3312 (ESP32-S3) pins

Source: RAKwireless RAK3312 datasheet + `pio/include/board_pins_rak3312.h`.

- **I2C1:** SDA=9, SCL=40 (SGP40 0x59, BMP280/BME280 0x76, SHTC3 0x70).
- **Display (RAK14000 SSD1680):** MOSI=11, SCLK=13, CS=12, DC=21, BUSY=42, PWR(WB_IO2)=14, RST=-1.
- **LED:** green=46 (heartbeat), blue=45.
- **SX1262 (on-module):** NSS=7, SCLK=5, MOSI=6, MISO=3, RESET=8, DIO1=47, BUSY=48, ANT_SW=4.
  RF config (validated): ANT_SW driven as RXEN (`USE_RXEN_ANT_PWR`), TCXO on. Tunable via
  `-DBOARD_RAK3312_USE_DIO2_ANTSW` / `-DBOARD_RAK3312_USE_TCXO`.
- **Battery ADC:** ESP32-S3 VBAT GPIO TBD → `APP_BATTERY_ADC_ENABLED=0` (3.95 V default) for now.

## ESP32 vs nRF52 notes

- ESP32 SPI/I2C buses are remappable: `hal_spi_begin(sclk,miso,mosi)` and `Wire.begin(sda,scl)`
  apply the pins on ESP32 (fixed by the variant on nRF52).
- `compat.h` uses the real ESP-IDF `esp_err_t`/codes on ESP32; the log markers
  (`result=PASS`, `STOP_AFTER_PASS`, ...) are identical on both boards.
- Serial routes to the ESP32-S3 native USB CDC (`ARDUINO_USB_CDC_ON_BOOT=1`).

## Deploy: continuous live publish

Gate 9 (`live_publish`) stops after one full sensor→display→uplink cycle (`live_publish_ok` /
`STOP_AFTER_PASS`) — that's the bring-up validation default. For a **deployed node** that keeps
reading the sensor + battery and uplinking to the gateway indefinitely, set in `platformio.ini`:

```ini
-DAPP_GATE=9
-DAPP_GATE9_CONTINUOUS=1     ; never halt; emits live_publish_running, uplink_ok climbs forever
```

Uplinks are CONFIRMED and spaced by `APP_GATE_UPLINK_PERIOD_MS` (20 s, AS923 duty-cycle safe);
each is acknowledged (`uplink_ack_ok`). Battery is in the payload (`battery_mv`); on RAK3312 it
reads the 3.95 V default until the ESP32-S3 VBAT ADC GPIO is wired (`APP_BATTERY_ADC_ENABLED=0`).
Revert both flags to `0` for bring-up. Validated on RAK3312 (DevEUI `CBAE62F5314DBB79`): join +
sustained 20 s-cadence confirmed uplinks, no halt.

## Credentials (per node)

Each physical node has its own DevEUI/AppKey. `inject_credentials.py` prefers
`firmware/.env.<env>` (e.g. `firmware/.env.rak3312`), falling back to `firmware/.env`. Provision a
distinct ChirpStack device per board with `tools/provision-node.sh` and keep its keys in the
matching per-board file (all gitignored).
