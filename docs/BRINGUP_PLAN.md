# BRINGUP PLAN (Canonical Gate 0..9 + Gate 2.1)

> **⚠ Historical (pre-P6 / PlatformIO-nRF52).** This doc predates the ESP-IDF/ESP32-S3 consolidation and may reference the retired `pio/` tree, `-DAPP_GATE`, or SX126x-Arduino. Canonical build/flash/provision instructions now live in [CLAUDE.md](../CLAUDE.md) (ESP-IDF `firmware/` + Kconfig).

## Operating Model

- Single selected gate per firmware run.
- Deterministic log tags only: `APP`, `I2C`, `DISPLAY`, `SENSOR`, `LORAWAN`, `FUOTA`.
- Stop-after-pass is mandatory.
- No auto-advance to next gate.

## Gate Map

- Gate 0: `env`
- Gate 1: `heartbeat`
- Gate 2: `display_smoke`
- Gate 2.1: `i2c_smoke` (internal `-DAPP_GATE=21`)
- Gate 3: `i2c_presence`
- Gate 4: `sensor_pipeline`
- Gate 5: `payload_v1`
- Gate 6: `lorawan_join_uplink`
- Gate 7: `reliability_buffer`
- Gate 8: `fuota_scaffold`
- Gate 9: `live_publish`

## Global Preconditions

1. Check `docs/11-pin-mapping-rak4630-rak19007.md` (source of truth: `pio/variants/WisCore_RAK4631_Board/variant.h`) before each gate.
2. Build from clean tree for each gate transition (`pio run -t clean && pio run`).
3. Record evidence in `docs/GATE_EXECUTION_LOG.md` and `docs/CHECKLIST_GATES_0_TO_9.md`.

## Gate 0: env

- Objective: toolchain + image boot sanity.
- Steps: set `-DAPP_GATE=0` in `pio/platformio.ini`, build, flash, monitor.
- Expected logs: `APP: boot ...`, `APP: result=PASS gate=0 name=env ...`
- Failure modes: no boot logs, reset loop, panic.
- PASS criteria: boot banner + PASS line present.

## Gate 1: heartbeat

- Objective: LED heartbeat and liveness counter.
- Steps: verify heartbeat pin mapping (`P1.03` Green LED), set `-DAPP_GATE=1`, run.
  - Reusable command: `examples/gates/run_gate_1_heartbeat.sh /dev/cu.usbmodem1101`
  - Detailed example: `examples/gates/GATE_1_HEARTBEAT_EXAMPLE.md`
- Expected logs: `APP: heartbeat_started ...`, `APP: result=PASS gate=1 ...`
- Failure modes: wrong pin, LED not blinking, loop not running.
- PASS criteria: visible LED blink + PASS line.

## Gate 2: display_smoke

- Objective: RAK14000 power-on + SSD1680 init + visible `HELLO WORLD`.
- Steps: verify display pin mapping (MOSI=P0.30, SCK=P0.03, CS=P0.26, DC=P0.17, BUSY=P0.04, PWR=P1.02), set `-DAPP_GATE=2`, run.
- Expected logs:
  - `DISPLAY: power_on wb_io2=...`
  - `DISPLAY: spi_bus_check spi_ok=1 ...`
  - `DISPLAY: init_ok panel=ssd1680 res=250x122`
  - `DISPLAY: hello_world_render_ok`
  - `APP: result=PASS gate=2 name=display_smoke ...`
- Failure modes: power rail disabled, BUSY timeout, SPI pin mismatch.
- PASS criteria: visible `HELLO WORLD` and PASS line.

## Gate 2.1: i2c_smoke

- Objective: isolate I2C bus sanity before full I2C presence and sensor pipeline gates.
- Steps:
  1. Ask connected device mode: `1=SGP40`, `2=BMP280`, `3=both`.
  2. Set `-DAPP_GATE2P1_EXPECTED_DEVICES=<1|2|3>`.
  3. Set `-DAPP_GATE=21` (Gate 2.1 alias), run.
- Expected logs:
  - `I2C: scan_found ...`
  - `APP: gate=2.1 name=i2c_smoke ...`
  - `APP: result=PASS gate=2.1 name=i2c_smoke ...`
- Failure modes: I2C pull-up/wiring mismatch, wrong sensor seating, wrong addresses.
- PASS criteria: selected expected devices detected in smoke gate.

## Gate 3: i2c_presence

- Objective: I2C init/scan and selected device presence.
- Steps:
  1. Ask connected device mode: `1=SGP40`, `2=BMP280`, `3=both`.
  2. Set `-DAPP_GATE3_EXPECTED_DEVICES=<1|2|3>`.
  3. Set `-DAPP_GATE=3`, run.
- Expected logs: `I2C: scan_found ...`, `APP: result=PASS gate=3 ...`
- Failure modes: wiring, wrong address, missing sensor power.
- PASS criteria: selected expected devices detected.

## Gate 4: sensor_pipeline

- Objective: identity checks + raw/sample read pipeline with range checks.
- Steps:
  1. Ask selected sensor mode: `1=SGP40`, `2=BMP280`, `3=both`.
  2. Set `-DAPP_GATE4_EXPECTED_DEVICES=<1|2|3>`.
  3. Set `-DAPP_GATE=4`, run.
- Expected logs: `SENSOR: identity...`, `APP: sensor_ok_count=...`, `APP: result=PASS gate=4 ...`
- Failure modes: SGP40 CRC error, BMP280 chip ID mismatch, out-of-range sample.
- PASS criteria: identity OK and stable valid sample pipeline.

## Gate 5: payload_v1

- Objective: deterministic 12-byte payload encoding.
- Steps: set `-DAPP_GATE=5`, run, verify hex + host tests.
- Expected logs: `APP: gate=5 payload_schema=1 payload_hex=...`, `APP: result=PASS gate=5 ...`
- Failure modes: payload length mismatch, vector mismatch.
- PASS criteria: on-device payload vector PASS + host payload test PASS.

## Gate 6: lorawan_join_uplink

- Objective: join/uplink control path with retries/counters.
- Steps:
  1. Ask for `DEVEUI` + `APPKEY` in `firmware/.env` (injected by `pio/scripts/inject_credentials.py`).
  2. Keep `JOINEUI=0000000000000000` unless overridden.
  3. Set `-DAPP_GATE=6`, run.
  4. Reusable command: `examples/gates/run_gate_6_lorawan_join_uplink.sh /dev/cu.usbmodem1101`
  5. Detailed example: `examples/gates/GATE_6_LORAWAN_JOIN_UPLINK_EXAMPLE.md`
- Backend: real SX126x-Arduino OTAA, AS923-1, ChirpStack v4, CONFIRMED (ACK-based) uplinks.
- Expected logs: `LORAWAN: join_start`, `LORAWAN: join_success`, `LORAWAN: uplink_sent_confirmed`, `LORAWAN: uplink_ack_ok`, `APP: result=PASS gate=6 ...`
- Failure modes: backend inactive, join retries without success.
- PASS criteria: joined + at least one uplink success.

## Gate 7: reliability_buffer

- Objective: buffer-before-join, flush-after-join behavior.
- Steps:
  1. Ask for `DEVEUI` + `APPKEY` in `firmware/.env`.
  2. Set `-DAPP_GATE=7`, run.
  3. Reusable command: `examples/gates/run_gate_7_reliability_buffer.sh /dev/cu.usbmodem1101`
  4. Detailed example: `examples/gates/GATE_7_RELIABILITY_BUFFER_EXAMPLE.md`
- Expected logs: `APP: gate=7 buffer_store ...`, `APP: gate=7 buffer_flush_ok ...`, `APP: result=PASS gate=7 ...`
- Failure modes: buffer never flushes, no recovery after backoff.
- PASS criteria: payload buffered and flushed after join.

## Gate 8: fuota_scaffold

- Objective: FUOTA hooks and rollback policy markers.
- Steps:
  1. Set `-DAPP_GATE=8`, run and capture FUOTA markers.
  2. Reusable command: `examples/gates/run_gate_8_fuota_scaffold.sh /dev/cu.usbmodem1101`
  3. Detailed example: `examples/gates/GATE_8_FUOTA_SCAFFOLD_EXAMPLE.md`
- Expected logs: `FUOTA: manifest=...`, `FUOTA: rollback_policy=hard_required`, `APP: result=PASS gate=8 ...`
- Failure modes: missing FUOTA markers.
- PASS criteria: all FUOTA markers present + PASS.

## Gate 9: live_publish

- Objective: integrated `sensor -> display -> uplink` loop.
- Steps:
  1. After Gate 8 PASS, ask selected mode: `1=SGP40`, `2=BMP280`, `3=both`.
  2. Set `-DAPP_GATE9_EXPECTED_DEVICES=<1|2|3>`.
  3. Ask for `DEVEUI` + `APPKEY` in `firmware/.env`.
  4. Set `-DAPP_GATE=9`, run.
- Expected logs:
  - `DISPLAY: render_data voc=... pressure=... temp=... batt=...`
  - `APP: result=PASS gate=9 name=live_publish ...`
- Failure modes: no valid sample, threshold never triggers display update, no uplink.
- PASS criteria: valid samples + >=1 display update + >=1 uplink success.
