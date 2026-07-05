#!/usr/bin/env python3
"""profile_to_blob.py — serialize a senseflow device profile JSON into the firmware NVS blob (ADR-006, v2).

Emits the exact byte format consumed by dp_deserialize() (firmware/components/device_profile):
multi-bus v2 header, big-endian, CRC-16/MODBUS trailer, floats as IEEE-754 bit patterns. The CRM
pushes the hex output to a node with `prov-profile <hex>`.

  python3 profile_to_blob.py profiles/bosch-bme280.json           # -> hex blob
  python3 profile_to_blob.py profiles/bosch-bme280.json --command  # -> "prov-profile <hex>"

The byte layout + enum codings MUST match device_profile.h/.c exactly (v2 header = 21 B:
version, device_byte, bus_kind, i2c_addr, sensor_type, baud(4), parity, stop_bits, default_fc,
default_word, scan_fc, scan_reg(2), scan_qty(2), total_len, n_meas, n_fields). For I2C profiles the
Modbus fields (baud/parity/scan/measurand registers) are zero — the read is done by the compiled
driver selected by sensor_type.
"""
import argparse
import json
import struct
import sys

BLOB_VERSION = 2
DTYPE = {"u16": 0, "i16": 1, "u32": 2, "i32": 3, "float32": 4}
WORD = {"ABCD": 0, "CDAB": 1, "BADC": 2, "DCBA": 3}
ENC = {"u8": 0, "i8": 1, "u16": 2, "i16": 3, "u32": 4, "i32": 5}
BUS = {"modbus_rtu": 0, "i2c": 1}
SENSOR = {"none": 0, "bme280": 1, "sgp40": 2, "shtc3": 3}


def crc16_modbus(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if (crc & 1) else (crc >> 1)
    return crc & 0xFFFF


def build_blob(p: dict) -> bytes:
    b = bytearray()
    b.append(BLOB_VERSION)
    b.append(p["device_byte"])
    b.append(p["bus_kind"])
    b.append(p["i2c_addr"])
    b.append(p["sensor_type"])
    b += struct.pack(">I", p["baud"])
    b.append(p["parity_byte"])
    b.append(p["stop_bits"])
    b.append(p["default_fc"])
    b.append(WORD[p["default_word"]])
    b.append(p["scan_fc"])
    b += struct.pack(">H", p["scan_reg"])
    b += struct.pack(">H", p["scan_qty"])
    b.append(p["total_len"])
    b.append(len(p["meas"]))
    b.append(len(p["fields"]))
    for m in p["meas"]:
        b += struct.pack(">H", m["reg"])
        b.append(m["fc"])
        b.append(DTYPE[m["type"]])
        b.append(WORD[m["word"]])
        b += struct.pack(">f", m["scale"])
        b += struct.pack(">f", m["offset"])
    for f in p["fields"]:
        b.append(f["value_index"])
        b.append(f["offset"])
        b.append(ENC[f["enc"]])
        b += struct.pack(">f", f["scale"])
    b += struct.pack(">H", crc16_modbus(bytes(b)))
    return bytes(b)


def profile_from_json(doc: dict) -> dict:
    """Normalize a device-profiles/*.json document into build_blob()'s input (multi-bus)."""
    bus, pl, meas = doc["bus"], doc["payload"], doc["measurands"]
    key_index = {m["key"]: i for i, m in enumerate(meas)}
    kind = bus["kind"]
    common = {
        "device_byte": doc["device"]["type_byte"],
        "bus_kind": BUS[kind],
        "total_len": pl["total_len"],
        "fields": [
            {
                "value_index": key_index[f["key"]],
                "offset": f["offset"],
                "enc": f["encoding"],
                "scale": float(f["scale"]),
            }
            for f in pl["fields"]
        ],
    }
    if kind == "i2c":
        return {
            **common,
            "i2c_addr": bus["addr"],
            "sensor_type": SENSOR[doc["device"]["sensor_type"]],
            "baud": 0, "parity_byte": 0, "stop_bits": 0, "default_fc": 0, "default_word": "ABCD",
            "scan_fc": 0, "scan_reg": 0, "scan_qty": 0,
            # I2C measurands carry only type/scale/offset (registers unused by the compiled reader).
            "meas": [
                {"reg": 0, "fc": 0, "type": m["type"], "word": "ABCD",
                 "scale": float(m["scale"]), "offset": float(m.get("offset", 0.0))}
                for m in meas
            ],
        }
    # modbus_rtu (careflow-compatible)
    dft, scan = doc["defaults"], doc["scan"]
    return {
        **common,
        "i2c_addr": 0,
        "sensor_type": 0,
        "baud": bus["baud"],
        "parity_byte": ord(bus["parity"]),
        "stop_bits": bus["stop_bits"],
        "default_fc": dft["function_code"],
        "default_word": dft["word_order"],
        "scan_fc": scan["function_code"],
        "scan_reg": scan["register"],
        "scan_qty": scan["quantity"],
        "meas": [
            {"reg": m["register"], "fc": m["function_code"], "type": m["type"],
             "word": m.get("word_order", dft["word_order"]),
             "scale": float(m["scale"]), "offset": float(m.get("offset", 0.0))}
            for m in meas
        ],
    }


def main() -> int:
    ap = argparse.ArgumentParser(description="Serialize a senseflow device profile JSON to an NVS blob (hex).")
    ap.add_argument("profile", help="path to a device-profiles/profiles/*.json")
    ap.add_argument("--command", action="store_true", help="print 'prov-profile <hex>'")
    args = ap.parse_args()
    blob = build_blob(profile_from_json(json.loads(open(args.profile).read())))
    print(f"prov-profile {blob.hex()}" if args.command else blob.hex())
    print(f"# {len(blob)} bytes, crc16=0x{blob[-2] << 8 | blob[-1]:04x}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
