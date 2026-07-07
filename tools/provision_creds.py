#!/usr/bin/env python3
"""provision_creds.py — write OTAA LoRaWAN credentials into a senseflow node's NVS over its
USB-Serial-JTAG provisioning console (P3). Drives the `esp> ` REPL that the CRM WebSerial flasher
also speaks:  prov creds <devEui> <joinEui> <appKey>  then  prov-done (restart into field mode).

Writes the NVS 'prov' namespace ADDITIVELY — the LoRaWAN nonces/session in the 'lorawan' namespace
are preserved (no DevNonce regression on re-join).

  python3 tools/provision_creds.py -p /dev/cu.usbmodem1301          # reads firmware/.env
  python3 tools/provision_creds.py -p /dev/cu.usbmodem1301 \
      --deveui 3cdc75fffe6f80e4 --joineui 0000000000000000 --appkey <32hex>

Credentials default to firmware/.env (gitignored): DEVEUI / JOINEUI / APPKEY (hex, no 0x).
The AppKey is NEVER printed. Cross-stack note: the CRM mints the AppKey (dev or production per the
selected stack); this tool only writes what it's given into NVS.
"""
import argparse
import os
import sys
import time

try:
    import serial  # pyserial (ships with the ESP-IDF python env)
except ImportError:
    sys.exit("ERROR: pyserial not found — run inside the ESP-IDF env "
             "(. ~/esp/esp-idf-v5.5.4/export.sh)")

DEFAULT_ENV = os.path.join(os.path.dirname(__file__), "..", "firmware", ".env")


def load_env(path):
    vals = {}
    if os.path.isfile(path):
        with open(path) as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith("#") and "=" in line:
                    k, v = line.split("=", 1)
                    vals[k.strip()] = v.strip().strip('"').strip("'")
    return vals


def main():
    ap = argparse.ArgumentParser(description="Provision OTAA creds into a senseflow node over esp>.")
    ap.add_argument("-p", "--port", required=True, help="serial port (e.g. /dev/cu.usbmodem1301)")
    ap.add_argument("--env", default=DEFAULT_ENV, help="path to firmware/.env")
    ap.add_argument("--deveui")
    ap.add_argument("--joineui")
    ap.add_argument("--appkey")
    ap.add_argument("--baud", type=int, default=115200)
    args = ap.parse_args()

    env = load_env(args.env)
    deveui = (args.deveui or env.get("DEVEUI", "")).lower()
    joineui = (args.joineui or env.get("JOINEUI", "0000000000000000")).lower()
    appkey = (args.appkey or env.get("APPKEY", "")).lower()

    if len(deveui) != 16 or len(joineui) != 16 or len(appkey) != 32:
        sys.exit("ERROR: need DEVEUI(16 hex) JOINEUI(16 hex) APPKEY(32 hex) — via --flags or "
                 f"firmware/.env ({args.env})")

    ser = serial.Serial(args.port, args.baud, timeout=0.4)
    ser.dtr = False
    ser.rts = False
    time.sleep(0.3)

    def send(line, secret=False, wait=1.2):
        ser.reset_input_buffer()
        for ch in line:
            ser.write(ch.encode())
            time.sleep(0.002)
        ser.write(b"\n")
        time.sleep(wait)
        out = ser.read(16384).decode(errors="replace")
        shown = (line.rsplit(" ", 1)[0] + " <redacted>") if secret else line
        print(f">> {shown}")
        for ln in out.splitlines():
            t = ln.strip()
            if t and not t.startswith("esp>") and (not secret or "appKey" in t or "OK" in t
                                                   or "ERR" in t.upper()):
                print("  ", t)
        return out

    send("")  # prime
    if "OK" not in send(f"prov creds {deveui} {joineui} {appkey}", secret=True):
        ser.close()
        sys.exit("ERROR: prov creds did not return OK")
    send("prov show")
    send("prov-done", wait=0.6)
    ser.close()
    print(f"\nprovisioned devEui={deveui} — node restarting into field mode.")


if __name__ == "__main__":
    main()
