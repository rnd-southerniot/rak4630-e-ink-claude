# Gate Run Instructions (Prompt Workflow)

## 1) Environment

```bash
export IDF_PYTHON_ENV_PATH=/Users/arif/.espressif/python_env/idf5.5_py3.14_env
source /Users/arif/esp/esp-idf/export.sh
```

## 2) Preflight (Always)

1. Open `docs/11-pin-mapping-rak3312-rak19007.md`.
2. Confirm the selected gate pin mapping.
3. Confirm serial port (example: `/dev/cu.usbmodem1101`).

## 3) Gate Selection

Canonical mode (default):

```bash
perl -pi -e 's/^CONFIG_APP_GATE_ID_SCHEME_NEW=.*/CONFIG_APP_GATE_ID_SCHEME_NEW=y/; s/^# CONFIG_APP_GATE_ID_SCHEME_LEGACY is not set/# CONFIG_APP_GATE_ID_SCHEME_LEGACY is not set/; s/^CONFIG_APP_GATE=.*/CONFIG_APP_GATE=<0-9|21>/' firmware/sdkconfig
```

`Gate 2.1` is selected by setting `CONFIG_APP_GATE=21`.

Legacy mode:

```bash
perl -pi -e 's/^# CONFIG_APP_GATE_ID_SCHEME_LEGACY is not set/CONFIG_APP_GATE_ID_SCHEME_LEGACY=y/; s/^CONFIG_APP_GATE_ID_SCHEME_NEW=.*/# CONFIG_APP_GATE_ID_SCHEME_NEW is not set/; s/^CONFIG_APP_GATE_LEGACY=.*/CONFIG_APP_GATE_LEGACY=<0-8>/' firmware/sdkconfig
```

## 4) Mandatory Gate Prompts

- Gate 1 (`heartbeat`): no sensor prompt. Heartbeat only.
  - Pin precheck: heartbeat primary `GPIO46` from `docs/11-pin-mapping-rak3312-rak19007.md`.
  - Reusable one-command run:
    - `examples/gates/run_gate_1_heartbeat.sh /dev/cu.usbmodem1101`
  - Detailed walkthrough:
    - `examples/gates/GATE_1_HEARTBEAT_EXAMPLE.md`
  - Set gate:

```bash
perl -pi -e 's/^CONFIG_APP_GATE=.*/CONFIG_APP_GATE=1/' firmware/sdkconfig
```

  - Expected Gate 1 markers:
    - `APP: heartbeat_started gpio=46`
    - `APP: result=PASS gate=1 name=heartbeat ...`
    - `APP: handshake=STOP_AFTER_PASS gate=1 ...`
- Gate 2 (`display_smoke`): no sensor prompt. Display only.
  - Current Gate 2 sequence:
    1. `DISPLAY: spi_bus_check ...` (SPI transaction sanity)
    2. `DISPLAY: hello_world_phase=white_clear`
    3. `DISPLAY: hello_world_phase=black_full`
    4. `DISPLAY: hello_world_phase=red_full`
    5. `DISPLAY: hello_world_phase=final_white_base`
    6. `DISPLAY: hello_world_phase=final_text_card`
    7. `DISPLAY: hello_world_render_ok`
  - Heartbeat LED is intentionally skipped on non-Gate1 runs:
    - `APP: heartbeat_skip gate=2 reason=non_heartbeat_gate`
  - Force tri-color RAK14000 profile before rerun (validated baseline):

```bash
perl -pi -e 's/^CONFIG_APP_DISPLAY_PIN_CS=.*/CONFIG_APP_DISPLAY_PIN_CS=12/; s/^CONFIG_APP_DISPLAY_PIN_SCLK=.*/CONFIG_APP_DISPLAY_PIN_SCLK=13/; s/^CONFIG_APP_DISPLAY_PIN_MOSI=.*/CONFIG_APP_DISPLAY_PIN_MOSI=11/; s/^CONFIG_APP_DISPLAY_PIN_MISO=.*/CONFIG_APP_DISPLAY_PIN_MISO=-1/; s/^CONFIG_APP_DISPLAY_PIN_DC=.*/CONFIG_APP_DISPLAY_PIN_DC=21/; s/^CONFIG_APP_DISPLAY_PIN_RST=.*/CONFIG_APP_DISPLAY_PIN_RST=-1/; s/^CONFIG_APP_DISPLAY_PIN_BUSY=.*/CONFIG_APP_DISPLAY_PIN_BUSY=42/; s/^CONFIG_APP_DISPLAY_PIN_PWR=.*/CONFIG_APP_DISPLAY_PIN_PWR=14/; s/^CONFIG_APP_DISPLAY_XRAM_OFFSET=.*/CONFIG_APP_DISPLAY_XRAM_OFFSET=0/; s/^# CONFIG_APP_DISPLAY_BUSY_ACTIVE_HIGH is not set/# CONFIG_APP_DISPLAY_BUSY_ACTIVE_HIGH is not set/; s/^CONFIG_APP_DISPLAY_PWR_INPUT_PULLUP=.*/CONFIG_APP_DISPLAY_PWR_INPUT_PULLUP=y/' firmware/sdkconfig
```

  - If a thin vertical edge strip appears, test alternate offset:

```bash
perl -pi -e 's/^CONFIG_APP_DISPLAY_XRAM_OFFSET=.*/CONFIG_APP_DISPLAY_XRAM_OFFSET=1/' firmware/sdkconfig
```

  - Expected Gate 2 markers:
    - `DISPLAY: power_on wb_io2=14 value=1 mode=input_pullup xram_offset=0`
    - `DISPLAY: spi_bus_check spi_ok=1 ...`
    - `DISPLAY: init_ok panel=ssd1680 res=250x122`
    - `DISPLAY: hello_world_render_ok`
    - `APP: result=PASS gate=2 name=display_smoke ...`
- Gate 2.1 (`i2c_smoke`): ask `1=SGP40`, `2=BMP280`, `3=both`, set:

```bash
perl -pi -e 's/^CONFIG_APP_GATE=.*/CONFIG_APP_GATE=21/; s/^CONFIG_APP_GATE2P1_EXPECTED_DEVICES=.*/CONFIG_APP_GATE2P1_EXPECTED_DEVICES=<1|2|3>/' firmware/sdkconfig
```

  - Expected Gate 2.1 markers:
    - `I2C: scan_found ...`
    - `APP: gate=2.1 name=i2c_smoke ...`
    - `APP: result=PASS gate=2.1 name=i2c_smoke ...`
- Gate 3 (`i2c_presence`): ask `1=SGP40`, `2=BMP280`, `3=both`, set:

```bash
perl -pi -e 's/^CONFIG_APP_GATE3_EXPECTED_DEVICES=.*/CONFIG_APP_GATE3_EXPECTED_DEVICES=<1|2|3>/' firmware/sdkconfig
```

  - Reusable one-command run:
    - `examples/gates/run_gate_3_i2c_presence.sh /dev/cu.usbmodem1101 1`
  - Detailed walkthrough:
    - `examples/gates/GATE_3_I2C_PRESENCE_EXAMPLE.md`
  - Expected Gate 3 markers:
    - `APP: gate=3 name=i2c_presence ...`
    - `I2C: scan_found addr=0x59` (SGP40 mode)
    - `APP: result=PASS gate=3 name=i2c_presence ...`

- Gate 4 (`sensor_pipeline`): ask `1=SGP40`, `2=BMP280`, `3=both`, set:

```bash
perl -pi -e 's/^CONFIG_APP_GATE4_EXPECTED_DEVICES=.*/CONFIG_APP_GATE4_EXPECTED_DEVICES=<1|2|3>/' firmware/sdkconfig
```

  - For real SGP40 data path (not mock), use `CONFIG_APP_GATE4_EXPECTED_DEVICES=1` and confirm:
    - `SENSOR: sgp40_data raw=... voc_index=...`
  - Reusable one-command run:
    - `examples/gates/run_gate_4_sensor_pipeline.sh /dev/cu.usbmodem1101 1`
  - Detailed walkthrough:
    - `examples/gates/GATE_4_SENSOR_PIPELINE_EXAMPLE.md`

- Gate 6, 7, 9: ask for OTAA credentials in `firmware/.env`:
  - `DEVEUI`
  - `APPKEY`
  - `JOINEUI=0000000000000000` (default)
  - Gate 6 reusable one-command run:
    - `examples/gates/run_gate_6_lorawan_join_uplink.sh /dev/cu.usbmodem1101`
  - Gate 6 detailed walkthrough:
    - `examples/gates/GATE_6_LORAWAN_JOIN_UPLINK_EXAMPLE.md`
  - Gate 7 reusable one-command run:
    - `examples/gates/run_gate_7_reliability_buffer.sh /dev/cu.usbmodem1101`
  - Gate 7 detailed walkthrough:
    - `examples/gates/GATE_7_RELIABILITY_BUFFER_EXAMPLE.md`
  - Gate 8 reusable one-command run:
    - `examples/gates/run_gate_8_fuota_scaffold.sh /dev/cu.usbmodem1101`
  - Gate 8 detailed walkthrough:
    - `examples/gates/GATE_8_FUOTA_SCAFFOLD_EXAMPLE.md`

- After Gate 8 PASS, before Gate 9: ask selected mode (`1|2|3`) and set:

```bash
perl -pi -e 's/^CONFIG_APP_GATE9_EXPECTED_DEVICES=.*/CONFIG_APP_GATE9_EXPECTED_DEVICES=<1|2|3>/' firmware/sdkconfig
```

  - Reusable one-command run:
    - `examples/gates/run_gate_9_live_publish.sh /dev/cu.usbmodem1101 1`
  - Detailed walkthrough:
    - `examples/gates/GATE_9_LIVE_PUBLISH_EXAMPLE.md`

## 5) Build / Flash / Monitor

```bash
idf.py -C firmware build
idf.py -C firmware -p /dev/cu.usbmodem1101 flash monitor
```

## 6) Evidence Capture

After each PASS, update:

- `docs/GATE_EXECUTION_LOG.md`
- `docs/CHECKLIST_GATES_0_TO_9.md`

Do not proceed until logs are captured.
