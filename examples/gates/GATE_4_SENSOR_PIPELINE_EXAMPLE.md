# Gate 4 Example (`sensor_pipeline`)

## Purpose

Validate sensor identity and sample pipeline before payload/uplink gates.

## Device Mode Prompt

Pick expected sensors before run:

- `1`: SGP40 only
- `2`: BMP280 only
- `3`: both SGP40 and BMP280

## Commands (Prompt/CLI)

```bash
/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh 4 --gate4-devices 1
/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh /dev/cu.usbmodem1101
```

One-command shortcut:

```bash
/Users/arif/rak4630-e-ink/examples/gates/run_gate_4_sensor_pipeline.sh /dev/cu.usbmodem1101 1
```

## Expected Markers (SGP40-only)

- `APP: boot ... gate=4 ... name=sensor_pipeline`
- `SENSOR: identity_sgp40 serial_ok=1 ...`
- `SENSOR: sgp40_data raw=... voc_index=...`
- `APP: result=PASS gate=4 name=sensor_pipeline ...`
- `APP: handshake=STOP_AFTER_PASS gate=4 ...`

## PASS Rule

Gate passes only when identity checks pass and `sensor_ok >= 3` with deterministic PASS marker.
