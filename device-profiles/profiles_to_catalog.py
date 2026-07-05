#!/usr/bin/env python3
"""profiles_to_catalog.py — generate top-level catalog summaries from profiles/ (consumed by the CRM).

Each profiles/<id>.json becomes a top-level <id>.json catalog entry (device byte, sensor type, bus,
payload size, and the ready-to-provision blob hex). `--check` fails if any catalog file is stale, so
CI keeps the CRM-facing catalog generated-fresh from the source profiles.
"""
import argparse
import glob
import json
import os
import sys

from profile_to_blob import build_blob, profile_from_json

HERE = os.path.dirname(os.path.abspath(__file__))


def catalog_entry(doc: dict) -> dict:
    return {
        "profileKey": doc["profile_id"],
        "model": doc["device"]["model"],
        "deviceByte": doc["device"]["type_byte"],
        "sensorType": doc["device"].get("sensor_type"),
        "bus": doc["bus"]["kind"],
        "measurands": [m["key"] for m in doc["measurands"]],
        "payloadBytes": doc["payload"]["total_len"],
        "blobHex": build_blob(profile_from_json(doc)).hex(),
    }


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate CRM catalog summaries from the profiles.")
    ap.add_argument("--check", action="store_true", help="fail if any catalog file is stale")
    args = ap.parse_args()
    profiles = sorted(glob.glob(os.path.join(HERE, "profiles", "*.json")))
    stale: list = []
    for p in profiles:
        doc = json.loads(open(p).read())
        text = json.dumps(catalog_entry(doc), indent=2) + "\n"
        out = os.path.join(HERE, doc["profile_id"] + ".json")
        if args.check:
            cur = open(out).read() if os.path.exists(out) else ""
            if cur != text:
                stale.append(doc["profile_id"])
        else:
            open(out, "w").write(text)
    if args.check:
        if stale:
            print("STALE catalog (run profiles_to_catalog.py to regenerate):", stale)
            return 1
        print(f"catalog fresh ({len(profiles)} profile(s))")
        return 0
    print(f"wrote {len(profiles)} catalog file(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
