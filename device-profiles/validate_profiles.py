#!/usr/bin/env python3
"""validate_profiles.py — schema + consistency checks for senseflow device profiles (multi-bus)."""
import glob
import json
import os
import sys

from profile_to_blob import BUS, ENC, SENSOR, build_blob, profile_from_json

ENC_SIZE = {"u8": 1, "i8": 1, "u16": 2, "i16": 2, "u32": 4, "i32": 4}
HERE = os.path.dirname(os.path.abspath(__file__))


def check(doc: dict, errors: list) -> None:
    pid = doc.get("profile_id", "?")
    dev, pl, bus, meas = doc["device"], doc["payload"], doc["bus"], doc["measurands"]
    keys = {m["key"] for m in meas}

    if pl["device_byte"] != dev["type_byte"]:
        errors.append(f"{pid}: payload.device_byte != device.type_byte")
    for f in pl["fields"]:
        if f["key"] not in keys:
            errors.append(f"{pid}: payload field key '{f['key']}' is not a measurand")
        if f["encoding"] not in ENC:
            errors.append(f"{pid}: bad encoding '{f['encoding']}'")
        if float(f["scale"]) == 0.0:
            errors.append(f"{pid}: field '{f['key']}' scale is 0")

    body_end = 3 + max((f["offset"] + ENC_SIZE[f["encoding"]] for f in pl["fields"]), default=0)
    if pl["total_len"] != body_end:
        errors.append(f"{pid}: total_len {pl['total_len']} != computed body end {body_end}")
    if pl["total_len"] > 53:
        errors.append(f"{pid}: total_len {pl['total_len']} > 53 (AS923 DR3 limit)")

    if bus["kind"] not in BUS:
        errors.append(f"{pid}: bad bus.kind '{bus['kind']}'")
    if bus.get("kind") == "i2c":
        if "addr" not in bus:
            errors.append(f"{pid}: i2c bus needs 'addr'")
        if dev.get("sensor_type") not in SENSOR:
            errors.append(f"{pid}: i2c needs a valid device.sensor_type {list(SENSOR)}")

    try:
        build_blob(profile_from_json(doc))
    except Exception as e:  # noqa: BLE001
        errors.append(f"{pid}: blob build failed: {e}")


def main() -> int:
    profiles = sorted(glob.glob(os.path.join(HERE, "profiles", "*.json")))
    if not profiles:
        sys.exit("no profiles found")
    errors: list = []
    seen: dict = {}
    for p in profiles:
        doc = json.loads(open(p).read())
        check(doc, errors)
        tb = doc["device"]["type_byte"]
        if tb in seen:
            errors.append(f"type_byte 0x{tb:02x} dup: {doc['profile_id']} & {seen[tb]}")
        seen[tb] = doc["profile_id"]

    try:
        import jsonschema  # noqa: E402
        schema = json.loads(open(os.path.join(HERE, "schema", "profile.schema.json")).read())
        for p in profiles:
            try:
                jsonschema.validate(json.loads(open(p).read()), schema)
            except jsonschema.ValidationError as e:
                errors.append(f"{os.path.basename(p)}: schema: {e.message}")
    except ImportError:
        print("(jsonschema not installed — skipping JSON-Schema validation)", file=sys.stderr)

    if errors:
        print("FAIL:")
        for e in errors:
            print("  -", e)
        return 1
    print(f"PASS: {len(profiles)} profile(s) valid")
    return 0


if __name__ == "__main__":
    sys.exit(main())
