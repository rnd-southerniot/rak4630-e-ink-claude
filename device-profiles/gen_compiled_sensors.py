#!/usr/bin/env python3
"""gen_compiled_sensors.py — emit compiled_sensors.json (senseflow's flashable-sensor manifest).

Analogous to careflow's compiled_readers.json (parsed from meter.h at Docker build time). A sensor is
"flashable" iff the firmware has a compiled read driver for it — the ground truth is the
``switch (p->sensor_type)`` in firmware/main/profile_reader.c: each ``case DP_SENSOR_<X>:`` that
reaches a real driver (not the ``default`` warn branch) means sensor_type ``<x>`` can be read on-target.

This scans profile_reader.c for those case labels and writes the lowercased sensor_type names as a JSON
array. The CRM firmware-service cross-references this against its SENSEFLOW_SENSOR_TO_BYTE map to compute
the ``flashable`` flag per profile — so adding a new ``case DP_SENSOR_FOO`` + rebuilding the image flips
foo to flashable with zero CRM change (mirrors careflow decision D-01).

Usage:
  gen_compiled_sensors.py                 # write device-profiles/compiled_sensors.json
  gen_compiled_sensors.py --check         # exit 1 if the committed manifest is stale (CI gate)
"""
import argparse
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
READER_SRC = os.path.join(REPO, "firmware", "main", "profile_reader.c")
OUT = os.path.join(HERE, "compiled_sensors.json")

# case DP_SENSOR_BME280:  ->  "bme280"
_CASE_RE = re.compile(r"case\s+DP_SENSOR_([A-Z0-9]+)\s*:")


def compiled_sensor_types(reader_src: str = READER_SRC) -> list[str]:
    """Lowercased sensor_type names dispatched to a compiled driver in profile_reader.c."""
    with open(reader_src) as fh:
        src = fh.read()
    # de-dup while preserving first-seen order, so the manifest is deterministic
    seen: dict[str, None] = {}
    for m in _CASE_RE.finditer(src):
        seen.setdefault(m.group(1).lower(), None)
    return list(seen)


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate/verify senseflow compiled_sensors.json.")
    ap.add_argument("--check", action="store_true", help="fail if committed manifest is stale")
    args = ap.parse_args()

    if not os.path.exists(READER_SRC):
        print(f"ERROR: reader source not found: {READER_SRC}", file=sys.stderr)
        return 2

    sensors = compiled_sensor_types()
    text = json.dumps(sensors, indent=2) + "\n"

    if args.check:
        cur = open(OUT).read() if os.path.exists(OUT) else ""
        if cur != text:
            print("STALE compiled_sensors.json (run gen_compiled_sensors.py):", sensors)
            return 1
        print(f"compiled_sensors.json fresh: {sensors}")
        return 0

    with open(OUT, "w") as fh:
        fh.write(text)
    print(f"wrote compiled_sensors.json: {sensors}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
