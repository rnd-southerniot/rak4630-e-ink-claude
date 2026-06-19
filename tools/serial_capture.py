#!/usr/bin/env python3
"""Bounded, non-interactive serial capture for gate bring-up.
Usage: serial_capture.py <port> <seconds> [open_timeout]
Waits up to open_timeout for the port to appear (it disappears during DFU reset),
reads lines for <seconds>, echoes them with a timestamp offset, then exits.
"""
import sys
import time
import serial  # pyserial, bundled in PlatformIO penv

port = sys.argv[1]
duration = float(sys.argv[2]) if len(sys.argv) > 2 else 15.0
open_timeout = float(sys.argv[3]) if len(sys.argv) > 3 else 10.0

# Wait for the port to (re)appear and open it.
ser = None
open_deadline = time.monotonic() + open_timeout
while time.monotonic() < open_deadline:
    try:
        ser = serial.Serial(port, 115200, timeout=0.2)
        break
    except Exception:
        time.sleep(0.3)
if ser is None:
    print(f"ERROR: could not open {port} within {open_timeout}s", flush=True)
    sys.exit(1)

start = time.monotonic()
deadline = start + duration
while time.monotonic() < deadline:
    try:
        line = ser.readline()
    except Exception as e:
        print(f"ERROR: read failed: {e}", flush=True)
        break
    if not line:
        continue
    t = time.monotonic() - start
    text = line.decode("utf-8", "replace").rstrip("\r\n")
    print(f"[{t:6.2f}] {text}", flush=True)
ser.close()
