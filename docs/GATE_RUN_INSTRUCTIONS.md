# Gate Run Instructions (Prompt Workflow)

## 1) Environment

PlatformIO is invoked directly; no shell environment export is required. All commands run from the repo `pio/` directory:

```bash
cd pio/
```

The PlatformIO binary lives at `~/.platformio/penv/bin/pio`. The first build performs a one-time board/variant install as described in the `pio/platformio.ini` header.

## 2) Preflight (Always)

1. Open `docs/11-pin-mapping-rak4630-rak19007.md` (source of truth: `pio/variants/WisCore_RAK4631_Board/variant.h` and `pio/include/board_pins.h`).
2. Confirm the selected gate pin mapping.
3. Confirm serial port (example: `/dev/cu.usbmodem1101`).

## 3) Gate Selection

Edit the `-DAPP_GATE=N` build flag in `pio/platformio.ini`. `N` is `0..9`, or `21` for Gate 2.1:

```ini
build_flags =
    -DAPP_GATE=21
```

`Gate 2.1` is selected by setting `-DAPP_GATE=21`. Per-gate device expectations are set with `-DAPP_GATE2P1_EXPECTED_DEVICES`, `-DAPP_GATE3_EXPECTED_DEVICES`, `-DAPP_GATE4_EXPECTED_DEVICES`, and `-DAPP_GATE9_EXPECTED_DEVICES` (`1=SGP40`, `2=BMP280`, `3=both`).

## 4) Mandatory Gate Prompts

- Gate 1 (`heartbeat`): no sensor prompt. Heartbeat only.
  - Pin precheck: heartbeat primary `P1.03` (Green LED) from `docs/11-pin-mapping-rak4630-rak19007.md`.
  - Reusable one-command run:
    - `examples/gates/run_gate_1_heartbeat.sh /dev/cu.usbmodem1101`
  - Detailed walkthrough:
    - `examples/gates/GATE_1_HEARTBEAT_EXAMPLE.md`
  - Set gate: edit `pio/platformio.ini` -> `-DAPP_GATE=1`.
  - Expected Gate 1 markers:
    - `APP: heartbeat_started pin=P1.03`
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
  - Display SPI pins (IO slot, fixed in `pio/include/board_pins.h`): MOSI=P0.30, SCK=P0.03, CS=P0.26, DC=P0.17, BUSY=P0.04, PWR=P1.02.
  - Expected Gate 2 markers:
    - `DISPLAY: power_on pwr=P1.02 value=1 ...`
    - `DISPLAY: spi_bus_check spi_ok=1 ...`
    - `DISPLAY: init_ok panel=ssd1680 res=250x122`
    - `DISPLAY: hello_world_render_ok`
    - `APP: result=PASS gate=2 name=display_smoke ...`
- Gate 2.1 (`i2c_smoke`): ask `1=SGP40`, `2=BMP280`, `3=both`, then set in `pio/platformio.ini`:

```ini
-DAPP_GATE=21
-DAPP_GATE2P1_EXPECTED_DEVICES=<1|2|3>
```

  - Expected Gate 2.1 markers:
    - `I2C: scan_found ...`
    - `APP: gate=2.1 name=i2c_smoke ...`
    - `APP: result=PASS gate=2.1 name=i2c_smoke ...`
- Gate 3 (`i2c_presence`): ask `1=SGP40`, `2=BMP280`, `3=both`, then set in `pio/platformio.ini`:

```ini
-DAPP_GATE=3
-DAPP_GATE3_EXPECTED_DEVICES=<1|2|3>
```

  - Reusable one-command run:
    - `examples/gates/run_gate_3_i2c_presence.sh /dev/cu.usbmodem1101 1`
  - Detailed walkthrough:
    - `examples/gates/GATE_3_I2C_PRESENCE_EXAMPLE.md`
  - I2C pins: SDA=P0.13, SCL=P0.14 (SGP40 at 0x59, BMP280 at 0x76).
  - Expected Gate 3 markers:
    - `APP: gate=3 name=i2c_presence ...`
    - `I2C: scan_found addr=0x59` (SGP40 mode)
    - `APP: result=PASS gate=3 name=i2c_presence ...`

- Gate 4 (`sensor_pipeline`): ask `1=SGP40`, `2=BMP280`, `3=both`, then set in `pio/platformio.ini`:

```ini
-DAPP_GATE=4
-DAPP_GATE4_EXPECTED_DEVICES=<1|2|3>
```

  - For real SGP40 data path (not mock), use `-DAPP_GATE4_EXPECTED_DEVICES=1` and confirm:
    - `SENSOR: sgp40_data raw=... voc_index=...`
  - Reusable one-command run:
    - `examples/gates/run_gate_4_sensor_pipeline.sh /dev/cu.usbmodem1101 1`
  - Detailed walkthrough:
    - `examples/gates/GATE_4_SENSOR_PIPELINE_EXAMPLE.md`

- Gate 6, 7, 9: ask for OTAA credentials in `firmware/.env` (gitignored, injected into the build by `pio/scripts/inject_credentials.py`):
  - `DEVEUI`
  - `APPKEY`
  - `JOINEUI=0000000000000000` (default)
  - LoRaWAN is real SX126x-Arduino OTAA, region AS923-1, ChirpStack v4; uplinks are CONFIRMED (ACK-based).
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

- After Gate 8 PASS, before Gate 9: ask selected mode (`1|2|3`) and set in `pio/platformio.ini`:

```ini
-DAPP_GATE=9
-DAPP_GATE9_EXPECTED_DEVICES=<1|2|3>
```

  - Reusable one-command run:
    - `examples/gates/run_gate_9_live_publish.sh /dev/cu.usbmodem1101 1`
  - Detailed walkthrough:
    - `examples/gates/GATE_9_LIVE_PUBLISH_EXAMPLE.md`

## 5) Build / Flash / Monitor

Run from the `pio/` directory. Serial monitor baud is `115200`:

```bash
cd pio/
~/.platformio/penv/bin/pio run                          # build
~/.platformio/penv/bin/pio run -t upload -t monitor     # flash + serial monitor
~/.platformio/penv/bin/pio run -t clean && ~/.platformio/penv/bin/pio run   # clean build
```

## 6) Evidence Capture

After each PASS, update:

- `docs/GATE_EXECUTION_LOG.md`
- `docs/CHECKLIST_GATES_0_TO_9.md`

Do not proceed until logs are captured.
