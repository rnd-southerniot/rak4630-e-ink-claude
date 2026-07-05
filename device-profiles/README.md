# senseflow device-profiles

Machine-readable **I²C sensor profiles** for `senseflow-eink-node` (ADR-006, multi-bus v2). One
profile drives the firmware's generic read + ADR-005 encode + the ChirpStack decoder — "select a
sensor → provision its profile → flash one generic binary", the same model as `careflow-modbus-node`
but for I²C sensors.

## Layout
- `profiles/*.json` — the source of truth (one file per sensor). Schema: `schema/profile.schema.json`.
- `profile_to_blob.py <profile>` — serialize a profile to the firmware NVS blob (hex). Byte-identical
  to the C `dp_serialize()` (v2: `[version][device_byte][bus_kind][i2c_addr][sensor_type]…`). The CRM
  pushes it with `prov-profile <hex>`.
- `profile_to_decoder.py -o chirpstack_senseflow_decoder.js` — generate the ChirpStack v4 codec (one
  `decodeUplink` branching on the payload device byte) so uplinks decode to named engineering fields.
- `validate_profiles.py` — schema + consistency checks (device-byte match, field↔measurand, total_len
  ≤ 53, unique type bytes, blob builds).
- `profiles_to_catalog.py [--check]` — generate/verify the top-level `<id>.json` catalog for the CRM.

## Compiled-driver hybrid (I²C)
An I²C profile carries `bus.kind=i2c`, `bus.addr`, a `device.sensor_type`, the measurand list, and the
ADR-005 payload table. Because sensors like the **BME280** need chip-specific compensation, the read is
done by a **compiled driver** in the firmware (`profile_reader.c` dispatches `sensor_type` →
`sensor_service_read_bmp280`) — the profile is *data* (labels, addr, payload mapping), the compensation
is *code*. Adding a sensor = one compiled driver + one profile JSON here.

## Device-byte registry (payload byte 1)
`0x10` BME280 · `0x11` SGP40 · `0x12` SHTC3 (senseflow I²C range; careflow Modbus uses `0x01`–`0x0F`).

## Verified
`profile_to_blob.py profiles/bosch-bme280.json` == the C `dp_serialize()` blob, and the generated
decoder turns a live uplink (`01 10 00 0a7d 26eb`) into `temp_C=26.85, pressure_hPa=996.3` — matching
the on-device BMP280 read.
