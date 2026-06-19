---
name: rak4630-serial-capture
description: Capture RAK4630 serial output non-interactively for gate verification. Use when pio device monitor fails with a TTY error or when scripting flash-then-capture of gate results.
---

# RAK4630 Serial Capture

`pio device monitor` needs an interactive TTY and **fails in a background/non-interactive
shell** with `termios.error: (19, 'Operation not supported by device')`. Use the bundled
pyserial reader instead.

## Capture

```bash
~/.platformio/penv/bin/python tools/serial_capture.py /dev/cu.usbmodem1101 <seconds> [open_timeout]
```

It waits for the port to (re)appear (the USB CDC re-enumerates during DFU), reads for
`<seconds>`, prints each line with a timestamp offset, then exits. `open_timeout` (default 10s)
covers the re-enumeration gap.

## Chain flash → capture (reliable boot capture)

The capture-on-reset window is hard to time. Chain the upload directly into the capture so the
reader catches the **fresh boot** after the DFU reset:

```bash
cd pio
~/.platformio/penv/bin/pio run -t upload --upload-port /dev/cu.usbmodem1101
~/.platformio/penv/bin/python ../tools/serial_capture.py /dev/cu.usbmodem1101 30 25
```

Filter for the markers you care about, e.g.:
```bash
... serial_capture.py /dev/cu.usbmodem1101 75 25 | grep -iE "LORAWAN|join|uplink|result=|boot board"
```

## Notes
- A failed `app_gate_init` drops the firmware into an LED error-blink loop that prints nothing
  — if a capture is empty after a flash, suspect a gate-init failure, reflash and capture the boot.
- Display (tri-color SSD1680) refresh is ~15s/phase, so gate 2/9 need long windows (90–120s).
- Find the port: `ls /dev/cu.usbmodem*`.
