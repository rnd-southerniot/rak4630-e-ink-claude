# Firmware (ESP-IDF, Gate Runner)

Target platform:

- Core: `RAK3312` (ESP32-S3 + SX1262)
- Base: `RAK19007`
- Display: `RAK14000` (SSD1680)
- Sensors: `RAK12047 (SGP40)` + external `BMP280` (I2C)
- Region: `AS923-1`

## Gate-Driven Execution

Gate selector:

- Canonical mode: `CONFIG_APP_GATE=0..9` plus `CONFIG_APP_GATE=21` for Gate 2.1 (`i2c_smoke`)
- Legacy mode: `CONFIG_APP_GATE_LEGACY=0..8` + scheme enable

Policy:

- Execute one gate only.
- On PASS, firmware emits `result=PASS gate=<N> ...` and idles.
- Move to next gate only after config change + reflash.

## Build and Flash

```bash
export IDF_PYTHON_ENV_PATH=/Users/arif/.espressif/python_env/idf5.5_py3.14_env
source /Users/arif/esp/esp-idf/export.sh
idf.py -C firmware build
idf.py -C firmware -p /dev/cu.usbmodem1101 flash monitor
```

Reusable gate examples:

- `examples/gates/GATE_1_HEARTBEAT_EXAMPLE.md`
- `examples/gates/GATE_2_DISPLAY_EXAMPLE.md`
- `examples/gates/GATE_2_1_I2C_EXAMPLE.md`
- `examples/gates/GATE_6_LORAWAN_JOIN_UPLINK_EXAMPLE.md`
- `examples/gates/GATE_7_RELIABILITY_BUFFER_EXAMPLE.md`
- `examples/gates/GATE_8_FUOTA_SCAFFOLD_EXAMPLE.md`
- `examples/gates/run_gate_1_heartbeat.sh`
- `examples/gates/run_gate_6_lorawan_join_uplink.sh`
- `examples/gates/run_gate_7_reliability_buffer.sh`
- `examples/gates/run_gate_8_fuota_scaffold.sh`

## LoRaWAN .env (Gate 6/7/9)

- Runtime credentials file: `firmware/.env`
- Template: `firmware/.env.example`
- Required values:
  - `DEVEUI`
  - `APPKEY`
- Default:
  - `JOINEUI=0000000000000000`

## Host Tests

```bash
tests/host/run_tests.sh
```

## Modules

- `app_gate.*`: gate state machine and stop-after-pass handshake
- `gate_id_map.*`: legacy-to-canonical gate translation
- `app_payload.*`: payload v1 encode + hex
- `heartbeat.*`: LED heartbeat task
- `i2c_bus.*`: I2C init/scan/probe
- `sensor_service.*`: sensor identity + sample pipeline
- `display_service.*`: render policy + real SSD1680 rendering
- `display_epd_ssd1680.*`: low-level SSD1680 transport
- `lorawan_service.*`: join/uplink stub and metrics
