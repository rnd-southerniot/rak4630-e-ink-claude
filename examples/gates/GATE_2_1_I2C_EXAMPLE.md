# Gate 2.1 Example (`i2c_smoke`, internal id `21`)

## Purpose

Run isolated I2C scan/probe without SPI display coupling.

## Device Mode Prompt

Pick expected devices before run:

- `1`: SGP40 only
- `2`: BMP280 only
- `3`: both SGP40 and BMP280

## Commands (Prompt/CLI)

```bash
examples/gates/set_gate_new.sh 2.1 --gate2p1-devices 3
examples/gates/run_gate.sh /dev/cu.usbmodem1101
```

One-command shortcut:

```bash
examples/gates/run_gate_2_1_i2c.sh /dev/cu.usbmodem1101 3
```

## Expected Markers

- `APP: gate=2.1 name=i2c_smoke ...`
- `I2C: scan_found ...`
- `APP: result=PASS gate=2.1 name=i2c_smoke ...`

## PASS Rule

Gate passes only if selected device mode is satisfied and PASS marker is logged.
