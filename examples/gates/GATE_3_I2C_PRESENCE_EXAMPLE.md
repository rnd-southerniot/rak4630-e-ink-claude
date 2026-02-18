# Gate 3 Example (`i2c_presence`)

## Purpose

Confirm expected I2C devices are present before sensor pipeline and LoRaWAN flow.

## Device Mode Prompt

Pick expected devices before run:

- `1`: SGP40 only
- `2`: BMP280 only
- `3`: both SGP40 and BMP280

## Commands (Prompt/CLI)

```bash
/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh 3 --gate3-devices 1
/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh /dev/cu.usbmodem1101
```

One-command shortcut:

```bash
/Users/arif/rak4630-e-ink/examples/gates/run_gate_3_i2c_presence.sh /dev/cu.usbmodem1101 1
```

## Expected Markers

- `APP: boot ... gate=3 ... name=i2c_presence`
- `I2C: init_ok ...`
- `I2C: scan_found addr=0x59` (SGP40 mode)
- `APP: result=PASS gate=3 name=i2c_presence ...`
- `APP: handshake=STOP_AFTER_PASS gate=3 ...`

## PASS Rule

Gate passes only if selected device mode is satisfied and PASS marker is logged.

## SGP40 Data Read Note

Gate 3 validates bus presence only.  
For actual SGP40 sensor data (`SENSOR: sgp40_data raw=... voc_index=...`), run Gate 4 in SGP40-only mode:

```bash
/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh 4 --gate4-devices 1
/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh /dev/cu.usbmodem1101
```
