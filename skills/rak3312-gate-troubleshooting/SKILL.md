---
name: rak3312-gate-troubleshooting
description: Troubleshoot canonical Gate 1..9 bring-up for RAK3312 + RAK19007 (heartbeat, display smoke, I2C presence, sensor pipeline, LoRaWAN, live publish).
---

# RAK3312 Gate Troubleshooting Workflow

Use this when canonical gate runs (`0..9`) fail.

## 1) Pin-Mapping Baseline

Always verify:

- `/Users/arif/rak4630-e-ink/docs/11-pin-mapping-rak3312-rak19007.md`

## 2) Gate 1 (Heartbeat)

1. Set `CONFIG_APP_GATE=1`.
2. Verify `CONFIG_APP_HEARTBEAT_GPIO=46` (fallback `45`).
3. Expected markers:
   - `APP: heartbeat_started gpio=...`
   - `APP: result=PASS gate=1 ...`

## 3) Gate 2 (Display Smoke)

1. Set `CONFIG_APP_GATE=2`.
2. Verify display pins and power-enable pin (`GPIO14`).
3. Expected markers:
   - `DISPLAY: power_on wb_io2=...`
   - `DISPLAY: init_ok panel=ssd1680 res=...`
   - `DISPLAY: hello_world_render_ok`
   - `APP: result=PASS gate=2 ...`

## 4) Gate 3 (I2C Presence)

1. Ask operator device mode: `1|2|3`.
2. Set `CONFIG_APP_GATE3_EXPECTED_DEVICES=<1|2|3>`.
3. Expected markers:
   - `I2C: scan_found addr=0x59` (if SGP40 required)
   - `I2C: scan_found addr=0x76` (if BMP280 required)
   - `APP: result=PASS gate=3 ...`

## 5) Gate 6/7/9 Credentials Rule

Before Gate 6, Gate 7, and Gate 9:

1. Ask for `DEVEUI` and `APPKEY`.
2. Use `JOINEUI=0000000000000000` default.
3. Keep values in `/Users/arif/rak4630-e-ink/firmware/.env` only.

## 6) Gate 9 Stuck Condition

If `sensor_ok`/`display_updates`/`uplink_ok` do not progress:

1. Confirm selected mode with `CONFIG_APP_GATE9_EXPECTED_DEVICES`.
2. Revalidate Gate 3 and Gate 4 for the same selected mode.
3. Verify display threshold/min refresh config is not blocking refresh.
4. Re-run Gate 9.
