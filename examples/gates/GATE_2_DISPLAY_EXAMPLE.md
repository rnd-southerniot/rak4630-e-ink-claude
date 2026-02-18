# Gate 2 Example (`display_smoke`)

## Purpose

Validate RAK14000 SPI path, power control, SSD1680 init, and visible full-screen color phases plus final text card.

## Commands (Prompt/CLI)

```bash
/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh 2
/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh /dev/cu.usbmodem1101
```

One-command shortcut:

```bash
/Users/arif/rak4630-e-ink/examples/gates/run_gate_2_display.sh /dev/cu.usbmodem1101
```

## Expected Markers

- `APP: gate=2 name=display_smoke ...`
- `DISPLAY: spi_bus_check spi_ok=1 ...`
- `DISPLAY: hello_world_phase=black_full`
- `DISPLAY: hello_world_phase=red_full`
- `DISPLAY: hello_world_phase=final_white_base`
- `DISPLAY: hello_world_phase=final_text_card`
- `DISPLAY: hello_world_render_ok`
- `APP: result=PASS gate=2 name=display_smoke ...`

## PASS Rule

Gate passes only if firmware logs PASS and user confirms visible `black -> red -> final text card` on panel.
