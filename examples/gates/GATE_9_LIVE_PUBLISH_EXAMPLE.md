# Gate 9 Example (`live_publish`)

## Purpose

Validate integrated runtime path for selected devices:

- sample sensors
- render on RAK14000
- uplink payload via LoRaWAN service path

## Device Mode Prompt

Pick expected sensors before run:

- `1`: SGP40 only
- `2`: BMP280 only
- `3`: both SGP40 and BMP280

## Prerequisites

- `/Users/arif/rak4630-e-ink/firmware/.env` exists with:
  - `DEVEUI`
  - `APPKEY`
  - `JOINEUI=0000000000000000` (default)

## Commands (Prompt/CLI)

```bash
/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh 9 --gate9-devices 1
/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh /dev/cu.usbmodem1101
```

One-command shortcut:

```bash
/Users/arif/rak4630-e-ink/examples/gates/run_gate_9_live_publish.sh /dev/cu.usbmodem1101 1
```

## Expected Markers (Mode `1`, SGP40-only)

- `APP: boot ... gate=9 ... name=live_publish`
- `SENSOR: sgp40_data raw=... voc_index=...`
- `DISPLAY: render_data voc=... pressure=... temp=... batt=...`
- `LORAWAN: join_success attempts=...`
- `LORAWAN: uplink_stub_ok bytes=12`
- `APP: result=PASS gate=9 name=live_publish ...`
- `APP: handshake=STOP_AFTER_PASS gate=9 ...`

## PASS Rule

Gate passes only when all three conditions are met:

- at least one display update
- joined state reached
- at least one uplink acknowledged by service path
