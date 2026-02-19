---
name: rak4630-gate-troubleshooting
description: Troubleshoot canonical Gate 0..9 bring-up for RAK4630 (nRF52840) + RAK19007 + RAK14000 on Arduino/PlatformIO.
---

# RAK4630 Gate Troubleshooting Workflow

Use this when canonical gate runs (`0..9`, `21`) fail on the nRF52840 PlatformIO build.

## 0) General Diagnostics

Before investigating any specific gate:

1. Confirm the serial monitor is connected at 115200 baud.
2. Check `pio/platformio.ini` has the correct `-DAPP_GATE=<N>` value.
3. Rebuild clean: `~/.platformio/penv/bin/pio run -t clean && ~/.platformio/penv/bin/pio run`
4. Verify power: WB_IO2 (pin 34 / P1.02) controls 3V3_S rail for sensors and display.

## 1) Gate 0 (env) — Toolchain Sanity

Instant pass. If this fails, the PlatformIO toolchain or board config is broken.

- Verify board JSON: `pio/boards/wiscore_rak4631.json`
- Verify variant: `pio/variants/WisCore_RAK4631_Board/variant.h`
- Expected: `result=PASS gate=0`

## 2) Gate 1 (heartbeat) — LED Toggle

1. Set `-DAPP_GATE=1` in platformio.ini.
2. LED is on PIN_LED1 (pin 35 / P1.03 Green).
3. Expected markers:
   - `APP: heartbeat_started`
   - `APP: result=PASS gate=1`
4. If LED not visible: check LED polarity (`LED_STATE_ON=1` means active HIGH).

## 3) Gate 2 (display_smoke) — SPI + SSD1680

1. Set `-DAPP_GATE=2`.
2. Verify IO slot SPI pins (from variant.h):
   - MOSI=pin 30 (P0.30), SCK=pin 3 (P0.03), CS=pin 26 (P0.26)
   - DC=WB_IO1 (pin 17 / P0.17), BUSY=WB_IO4 (pin 4 / P0.04)
   - PWR=WB_IO2 (pin 34 / P1.02) — must be HIGH before SPI init
3. Expected markers:
   - `DISPLAY: power_on`
   - `DISPLAY: init_ok panel=ssd1680`
   - `DISPLAY: hello_world_render_ok`
   - `APP: result=PASS gate=2`
4. If BUSY timeout: check `APP_DISPLAY_BUSY_ACTIVE_HIGH` polarity.
5. If no display output: verify RAK14000 is in IO slot (not sensor slot).

## 4) Gate 21 (i2c_smoke) — I2C Bus Scan

1. Set `-DAPP_GATE=21`.
2. I2C pins: SDA=pin 13 (P0.13), SCL=pin 14 (P0.14), 100kHz.
3. Expected markers:
   - `I2C: scan_found addr=0x59` (SGP40)
   - `I2C: scan_found addr=0x76` (BMP280)
   - `APP: result=PASS gate=21`
4. If no devices found: check WB_IO2 power enable is HIGH (3V3_S rail).

## 5) Gate 3 (i2c_presence) — Device Probe

1. Set `-DAPP_GATE=3`.
2. Set `-DAPP_GATE3_EXPECTED_DEVICES=<1|2|3>`:
   - 1 = SGP40 only (0x59)
   - 2 = BMP280 only (0x76)
   - 3 = both
3. Expected: `APP: result=PASS gate=3`

## 6) Gate 4 (sensor_pipeline) — Sensor Read

1. Set `-DAPP_GATE=4`, `-DAPP_GATE4_EXPECTED_DEVICES=<1|2|3>`.
2. Expected markers:
   - `SENSOR: sgp40_identity_ok` and/or `SENSOR: bmp280_identity_ok`
   - `SENSOR: sample voc_index=... pressure_pa=... temperature_c=...`
   - `APP: result=PASS gate=4`
3. If BMP280 reads implausible values: may need calibration warm-up (2-3 samples).

## 7) Gate 5 (payload_v1) — Encode Test

1. Set `-DAPP_GATE=5`.
2. Pure software test — no hardware dependencies.
3. Expected: `APP: result=PASS gate=5`
4. Also runnable on host: `tests/host/run_tests.sh`

## 8) Gates 6/7/9 — LoRaWAN Credentials

Before Gate 6, Gate 7, and Gate 9:

1. LoRaWAN service is currently a **stub** in the PlatformIO port.
2. Real SX126x-Arduino OTAA integration requires:
   - Call `lora_rak4630_init()` before `lmh_init()`
   - Set credentials via `lmh_setDevEui()`, `lmh_setAppEui()`, `lmh_setAppKey()`
   - Use `LORAMAC_REGION_AS923` for Bangladesh
3. DEVEUI and APPKEY: keep in `firmware/.env` (gitignored).

## 9) Gate 9 Stuck Condition

If `sensor_ok`/`display_updates`/`uplink_ok` do not progress:

1. Confirm `-DAPP_GATE9_EXPECTED_DEVICES` matches connected sensors.
2. Revalidate Gate 3 and Gate 4 for the same device mode.
3. Check display refresh thresholds (`APP_DISPLAY_MIN_REFRESH_SEC`, deltas).
4. Check LoRaWAN stub behavior — uplink will auto-pass in stub mode.
5. Re-run Gate 9.

## Quick Reference: Pin Map

| Function | Pin | nRF52840 GPIO | Notes |
|----------|-----|---------------|-------|
| LED Green | 35 | P1.03 | Heartbeat |
| LED Blue | 36 | P1.04 | Connection indicator |
| I2C SDA | 13 | P0.13 | Sensor bus |
| I2C SCL | 14 | P0.14 | Sensor bus |
| SPI MOSI | 30 | P0.30 | IO slot (E-Ink) |
| SPI SCK | 3 | P0.03 | IO slot (E-Ink) |
| SPI CS | 26 | P0.26 | IO slot (E-Ink) |
| EPD DC | 17 | P0.17 | WB_IO1 |
| EPD BUSY | 4 | P0.04 | WB_IO4 |
| EPD PWR | 34 | P1.02 | WB_IO2 (3V3_S) |
