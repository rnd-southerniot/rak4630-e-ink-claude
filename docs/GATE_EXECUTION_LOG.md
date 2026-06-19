# Gate Execution Log (Canonical 0..9 + Gate 2.1)

> **All evidence below the "RAK4630 nRF52840 Bring-Up" section is from the legacy
> ESP-IDF / RAK3312 (ESP32-S3) hardware and is SUPERSEDED.** The firmware was
> ported to Arduino/PlatformIO on RAK4630 (nRF52840) in Feb 2026; that port
> invalidates the old PASS records (different MCU, pins, and serial markers).
> Capture fresh evidence in the session block immediately below.

## RAK4630 nRF52840 Bring-Up (2026-06) — ACTIVE

Build/flash/monitor (PlatformIO, from `pio/`):
`~/.platformio/penv/bin/pio run -t upload -t monitor`.
Gate selection: `-DAPP_GATE=N` in `pio/platformio.ini` (or
`examples/gates/set_gate_new.sh N`).

### Run Metadata

- Run date: 2026-06-19
- Operator: Arif
- Firmware commit: `24bc114` (working tree: real-OTAA rework, uncommitted)
- Serial port: `/dev/cu.usbmodem1101`
- Sensors wired (`1`=SGP40, `2`=BMP280, `3`=both): `2` (BME280, chip_id 0x60, hand-wired to baseboard I2C). Original SGP40 breakout was faulty. Gates 2.1/3/4/9 use devices=2.
- ChirpStack device provisioned (DEVEUI in `firmware/.env`): `NO` (pending — required before gates 6/7/9)
- RAK7266 gateway online in ChirpStack (AS923): `TBD`

### Pre-flight (host + build) — verified 2026-06-19

- Host tests: `PASS payload_encode_v1` + `PASS gate_id_legacy_map`.
- Clean build `APP_GATE=0`: SUCCESS (PlatformIO, board `wiscore_rak4631`).
- LoRaWAN backend build `APP_GATE=6` / `APP_GATE=9`: SUCCESS (real SX126x-Arduino
  OTAA linked; flash ~20% vs ~10% for stub).

### Gate Evidence (capture on hardware)

| Gate | Config | Result (`PASS`/`FAIL`) | Key markers / notes |
|------|--------|------------------------|---------------------|
| 0 `env` | `APP_GATE=0` | PASS | `gate=0 halted_after_pass=1` (toolchain/boot OK on nRF52840) |
| 1 `heartbeat` | `APP_GATE=1` | PASS | `heartbeat_started gpio=35` (P1.03); `result=PASS gate=1 heartbeat_ok toggles=6`; Green LED blink visually confirmed by operator |
| 2 `display_smoke` | `APP_GATE=2` | PASS (fw) | Two fixes (one at a time): (1) `BUSY_ACTIVE_HIGH=0→1` fixed `sw_reset_busy_timeout`→`init_ok`; (2) refresh timeout `5000→20000ms` (`APP_DISPLAY_REFRESH_TIMEOUT_MS`) fixed tri-color refresh (~15s/phase). `hello_world_render_ok`; `result=PASS gate=2`. HELLO/WORLD text visually confirmed by operator. |
| 2.1 `i2c_smoke` | `APP_GATE=21`, devices=`2` | PASS | With BMP280 (replaced faulty SGP40 breakout): `scan_found addr=0x76`; `found=1 bmp280=1`; `result=PASS gate=2.1`. Confirms fw fixes were correct (sensor rail WB_IO2/P1.02 + nRF52 internal pull-ups `int_pullups=on`). Earlier SGP40 `found=0` was a faulty/mis-wired non-RAK breakout — same bus reads BMP280 fine. |
| 3 `i2c_presence` | `APP_GATE=3`, devices=`2` | PASS | `scan_found addr=0x76`; `result=PASS gate=3 presence_ok expected=2` |
| 4 `sensor_pipeline` | `APP_GATE=4`, devices=`2` | PASS | fw fix: accept BME280 chip_id `0x60` (+0x58 BMP280) in `sensor_service.cpp`. `identity_bmp280 chip_id=0x60 chip_id_ok=1`; live `bmp280_data pressure=100120Pa temperature=31.44C`; `result=PASS gate=4 sensor_ok=3 sensor_fail=0` |
| 5 `payload_v1` | `APP_GATE=5` | PASS | `payload_hex=01 27 10 00 01 8A 24 0B 54 0F A0 01`; `result=PASS gate=5 payload_encode_ok bytes=12` (matches host vector) |
| 6 `lorawan_join_uplink` | `APP_GATE=6` (+creds) | PASS | Device auto-provisioned via CRM onboarding workflow → ChirpStack 140 (task `cmqkvcmqa…`, DevEUI `F815C58DC40AC898`). `inject_credentials: injected …`; real `join_success attempts=1` (proves RAK7266 gateway live); `uplink_sent_confirmed` → `uplink_ack_ok uplink_ok=1` (confirmed downlink ACK); `result=PASS gate=6 joined=1 uplink_done=1` |
| 7 `reliability_buffer` | `APP_GATE=7` (+creds) | PASS | `backend_active=0` → `buffer_store buffered=1`; after delay `backend_active=1` → `join_success` → `buffer_flush_ok flushed_count=1`; `result=PASS gate=7 buffered=1 flushed=1`; `uplink_ack_ok` |
| 8 `fuota_scaffold` | `APP_GATE=8` | PASS | `manifest=placeholder-v1 region=AS923-1`; `fuota_scaffold_ready hooks=1 rollback=1`; `result=PASS gate=8` |
| 9 `live_publish` | `APP_GATE=9`, devices=`2` (+creds) | PASS | Full cycle: live `bmp280_data` → `join_success` → `render_data voc=100.00 pressure=100077 temp=31.44 batt=4.10` → `uplink_sent_confirmed` → `uplink_ack_ok`; `result=PASS gate=9 sensor_ok=6 display_updates=1 uplink_ok=1`. With LiPo + `APP_BATTERY_ADC_ENABLED=1`: real `battery_adc raw=3232 battery_v=4.095` after fixing `PIN_BATTERY_VBAT` (was P1.00 = no ADC → 0; now WB_A0/P0.05/AIN3 per RAK example). VOC seeded 100 (no SGP40). |

> Gate 6 & 9 PASS criteria require a **confirmed-uplink ACK** (`uplink_ack_ok`),
> not just transmit — verify the frame + 12-byte payload in the ChirpStack device
> event log (10.10.8.140).

---

### Sensor-registry refactor re-validation (2026-06-19)

After the pluggable sensor-driver registry + SHTC3 driver + payload v2 refactor, re-ran with
BME280 (devices=2, no SHTC3 module) to confirm no regression:

- Gate 4: `registry_ready drivers=3 present=1 shtc3=0`; `identity_bmp280 chip_id=0x60 chip_id_ok=1`;
  `bmp280_data`; `result=PASS gate=4 sensor_ok=3 sensor_fail=0`.
- Gate 9: `registry_ready drivers=3 present=1`; `join_success`; `render_data … humidity=0.0 batt=4.13`;
  `uplink_ack_ok`; `result=PASS gate=9 sensor_ok=6 display_updates=1 uplink_ok=1`.

SHTC3 (0x70) probed absent → no effect; its read/humidity path is code-only pending a module.
Host tests: `PASS payload_encode_v1`, `PASS payload_encode_v2`, `PASS gate_id_legacy_map`.

## Legacy ESP-IDF / RAK3312 Evidence (SUPERSEDED)

## Fresh Rerun Requirement

This log tracks the required full rerun after inserting Gate 2 (`display_smoke`) and adding Gate 2.1 (`i2c_smoke`) in canonical mode.

## Run Metadata

- Run date:
- Operator:
- Hardware profile (`1|2|3` where relevant):
- Firmware commit:
- Serial port:

## Canonical Gate Evidence

### Gate 0 (`env`)

- Config:
- Evidence lines:
- Result: `PASS|FAIL`

### Gate 1 (`heartbeat`)

- Config:
- User visual confirmation (LED blinking):
- Evidence lines:
- Result: `PASS|FAIL`

### Gate 2 (`display_smoke`)

- Config:
- Visible `HELLO WORLD` confirmation:
- Evidence lines:
  - `DISPLAY: power_on wb_io2=...`
  - `DISPLAY: init_ok panel=ssd1680 res=...`
  - `DISPLAY: hello_world_render_ok`
- Result: `PASS|FAIL`

### Gate 2.1 (`i2c_smoke`, internal gate id `21`)

- Prompt answer (`1|2|3`):
- Config (`CONFIG_APP_GATE2P1_EXPECTED_DEVICES`):
- Evidence lines:
  - `I2C: scan_found ...`
  - `APP: result=PASS gate=2.1 name=i2c_smoke ...`
- Result: `PASS|FAIL`

### Gate 3 (`i2c_presence`)

- Prompt answer (`1|2|3`):
- Config (`CONFIG_APP_GATE3_EXPECTED_DEVICES`):
- Evidence lines:
- Result: `PASS|FAIL`

### Gate 4 (`sensor_pipeline`)

- Prompt answer (`1|2|3`):
- Config (`CONFIG_APP_GATE4_EXPECTED_DEVICES`):
- Evidence lines:
- Result: `PASS|FAIL`

### Gate 5 (`payload_v1`)

- Config:
- Evidence lines:
- Host test result:
- Result: `PASS|FAIL`

### Gate 6 (`lorawan_join_uplink`)

- Credentials source: `firmware/.env`
- Evidence lines:
- Result: `PASS|FAIL`

### Gate 7 (`reliability_buffer`)

- Credentials source: `firmware/.env`
- Evidence lines:
- Result: `PASS|FAIL`

### Gate 8 (`fuota_scaffold`)

- Config:
- Evidence lines:
- Result: `PASS|FAIL`

### Gate 9 (`live_publish`)

- Prompt answer (`1|2|3`):
- Config (`CONFIG_APP_GATE9_EXPECTED_DEVICES`):
- Credentials source: `firmware/.env`
- Visible sensor render confirmation:
- Evidence lines:
  - `DISPLAY: render_data ...`
  - `APP: result=PASS gate=9 name=live_publish ...`
- Result: `PASS|FAIL`

## Legacy Mode (Optional)

If legacy IDs are used, include mapping evidence line:

- `APP: gate_id_scheme=legacy input=<old> resolved=<new>`

## Build Verification (2026-02-16)

- Host tests:
  - `PASS payload_encode_v1 deterministic vector`
  - `PASS gate_id_legacy_map deterministic vector`
- Firmware build matrix:
  - Gate `2` + panel `SSD1680_250X122`: PASS
  - Gate `21` (Gate `2.1`) + panel `SSD1680_250X122`: PASS
- Note:
  - CMake warns about `ESP_ROM_ELF_DIR` missing in this shell setup, but build/link completes successfully.
- Pending build checks:
  - Gate `9` + panel `SSD1680_250X122`
  - Gate `2` + panel `SSD1680_212X104`
  - Gate `9` + panel `SSD1680_212X104`
- Hardware rerun Gate `1..9`: pending operator execution and evidence capture.

## Fresh Rerun Session (2026-02-16)

### Gate 1 (`heartbeat`)

- Config: `CONFIG_APP_GATE=1`
- Evidence:
  - `APP: heartbeat_started gpio=46`
  - `APP: result=PASS gate=1 name=heartbeat heartbeat_ok toggles=7`
  - `APP: handshake=STOP_AFTER_PASS gate=1 ...`
- User visual confirmation: `YES` (LED blinking)
- Result: `PASS`

### Gate 2 (`display_smoke`) attempt A

- Config: `CONFIG_APP_GATE=2`
- Evidence:
  - `DISPLAY: power_on wb_io2=14 value=1`
  - `DISPLAY: ... sw_reset_busy_timeout`
  - `APP: ... display_init_failed`
- Result: `FAIL`
- Action:
  - Added BUSY polarity control (`CONFIG_APP_DISPLAY_BUSY_ACTIVE_HIGH`, default active-low)
  - Added SSD1680 native geometry mapping (native `122x250` for `250x122` profile)

### Gate 2 (`display_smoke`) attempt B

- Config: `CONFIG_APP_GATE=2`
- Evidence:
  - `DISPLAY: power_on wb_io2=14 value=1`
  - `DISPLAY: init_ok panel=ssd1680 res=250x122 orientation=landscape native=122x250`
  - `DISPLAY: hello_world_render_ok`
  - `APP: result=PASS gate=2 name=display_smoke ...`
  - `APP: handshake=STOP_AFTER_PASS gate=2 ...`
- User visual confirmation: pending
- Result: firmware `PASS`, visual pending

### Gate 2 (`display_smoke`) attempt C

- Config: `CONFIG_APP_GATE=2`
- Firmware evidence:
  - `DISPLAY: init_ok panel=ssd1680 res=250x122 orientation=landscape native=122x250`
  - `DISPLAY: hello_world_phase=black_fill`
  - `DISPLAY: hello_world_phase=white_text`
  - `DISPLAY: hello_world_render_ok`
  - `APP: result=PASS gate=2 ...`
- User visual confirmation: pending (requested explicit observation of black flash / text)
- Result: firmware `PASS`, visual pending

### Gate 2 (`display_smoke`) SPI probe run (2026-02-16)

- Purpose: emulate SPI device "scan" via SSD1680 BUSY-line response probing.
- Pin mapping log:
  - `DISPLAY: pinmap cs=12 clk=13 mosi=11 miso=10 dc=42 rst=21 busy=38 pwr=14`
- Probe evidence:
  - `DISPLAY: probe_reset_busy_pulse seen=0`
  - `DISPLAY: probe_refresh_busy_pulse seen=0`
  - `DISPLAY: probe_busy reset_seen=0 refresh_seen=0 busy_level=1`
  - `DISPLAY: probe_fail no_refresh_busy_pulse`
- Interpretation:
  - SPI write path is executing, but panel BUSY never responds; display is not electrically responding on BUSY line.
- Result:
  - `FAIL` (hardware response missing)

### Gate 2 (`display_smoke`) BUSY polarity cross-check

- Test A: `busy_active_high=0` (active-low)
  - `probe_reset_busy_pulse seen=0`
  - `probe_refresh_busy_pulse seen=0`
  - `probe_busy ... busy_level=1`
  - No panel response detected.
- Test B: `busy_active_high=1` (active-high)
  - `sw_reset_busy_timeout` during init.
- Conclusion:
  - Keep active-low configuration, but panel still does not respond on BUSY.
  - This indicates a physical connection/module-level issue rather than SPI command flow in firmware.

### Gate 2 (`display_smoke`) power-line diagnostic

- Pin mapping probe line:
  - `DISPLAY: pinmap cs=12 clk=13 mosi=11 miso=10 dc=42 rst=21 busy=38 pwr=14`
- Power/BUSY line evidence:
  - `DISPLAY: busy_cfg ... active_high=0 ... pwr_active_high=1 pwr_level=0`
  - `DISPLAY: probe_reset_busy_pulse seen=0`
  - `DISPLAY: probe_refresh_busy_pulse seen=0`
- Interpretation:
  - Requested display-power enable is not observed at expected level and no BUSY activity occurs.
  - Display likely not electrically attached/enabled on IO path (seat/orientation/FPC or module/hardware path issue).

### Gate 2 (`display_smoke`) mapping profile retest (DC=21, RST=-1, BUSY=42)

- Pin profile used:
  - `DISPLAY: pinmap cs=12 clk=13 mosi=11 miso=10 dc=21 rst=-1 busy=42 pwr=14`
- Diagnostic result:
  - `probe_reset_busy_pulse seen=0`
  - `probe_refresh_busy_pulse seen=0`
  - `probe_fail no_refresh_busy_pulse`
- Conclusion:
  - No electrical response from panel BUSY line even with official-style mapping profile.

### Gate 2 (`display_smoke`) post-reseat retest

- User action: RAK14000 reseated, then Gate 2 rerun.
- Evidence:
  - `DISPLAY: pinmap cs=12 clk=13 mosi=11 miso=10 dc=21 rst=-1 busy=42 pwr=14`
  - `DISPLAY: probe_reset_busy_pulse seen=0`
  - `DISPLAY: probe_refresh_busy_pulse seen=0`
  - `DISPLAY: probe_fail no_refresh_busy_pulse`
- Result:
  - `FAIL` persists after reseat.
- Assessment:
  - Display module/path still non-responsive electrically (BUSY never toggles).

### Gate 2 (`display_smoke`) tri-color workflow update

- Added tri-color render phases for 2.13\" panel:
  - `hello_world_phase=black_fill`
  - `hello_world_phase=red_fill`
  - `hello_world_phase=tri_text`
- BUSY fallback mode active:
  - `probe_warn no_refresh_busy_pulse_using_time_fallback=1`
- Gate status:
  - `APP: result=PASS gate=2 ...`
- Visual confirmation:
  - pending user confirmation for visible black/red transitions and text.

### Gate 2 (`display_smoke`) tri-color stabilization run

- Panel profile: 2.13\" tri-color 250x122
- Render sequence:
  - `hello_world_phase=white_clear`
  - `hello_world_phase=tri_test`
- Timing:
  - BUSY-missing fallback delay = `10000 ms` per refresh cycle
  - Total gate render window ~20 seconds
- Evidence:
  - `APP: result=PASS gate=2 ...`
- User visual status: pending after stabilized tri-color sequence.

### Gate 2 (`display_smoke`) noisy-screen corrective patch (in progress)

- User report:
  - Display still noisy/static red-white after prior PASS markers.
- Firmware corrective changes applied:
  - Aligned power control to RAK sample behavior:
    - `WB_IO2` (`GPIO14`) supports `INPUT_PULLUP` power-enable mode.
  - Added explicit SPI pin config (`MOSI/SCLK/MISO`) and set tri-color default `MISO=-1`.
  - Added SSD1680 startup settings used by RAK/Adafruit path:
    - VCOM (`0x2C=0x36`)
    - Gate voltage (`0x03=0x17`)
    - Source voltage (`0x04=0x41,0x00,0x32`)
    - X RAM offset default `1`
  - Fixed BUSY wait timing to catch delayed BUSY assertion windows.
  - Full-refresh control changed to `0xF4` (tri-color full update baseline).
- Build status:
  - `idf.py build` PASS after patch.
- Next action:
  - Reflash Gate 2 and capture visible outcome + logs with new markers (`mode=input_pullup`, `xram_offset=1`).

### Gate 2 (`display_smoke`) focused recovery mode (SPI check + white clear)

- Requested debug actions:
  1. Check SPI bus first.
  2. Stop onboard heartbeat LED blinking.
  3. Clear display to white only.
- Firmware update:
  - Gate 2 now runs `display_service_spi_bus_check()` before render.
  - Gate 2 render changed to `display_service_render_white_clear()` (no text draw).
  - Heartbeat task starts only in Gate 1; non-Gate1 logs `heartbeat_skip`.
- Expected evidence for this run:
  - `APP: heartbeat_skip gate=2 reason=non_heartbeat_gate`
  - `DISPLAY: spi_bus_check spi_ok=1 tx_calls=... tx_bytes=...`
  - `DISPLAY: white_clear_phase=1`
  - `DISPLAY: white_clear_phase=2`
  - `DISPLAY: white_clear_ok`
  - `APP: result=PASS gate=2 name=display_smoke ... mode=white_clear ...`

### Gate 2 (`display_smoke`) edge-strip tuning: `xram_offset=0`

- Reason:
  - Panel became mostly white, but a thin residual strip remained on one edge.
- Tuning action:
  - Set `CONFIG_APP_DISPLAY_XRAM_OFFSET=0` and reran Gate 2 white-clear.
- Evidence:
  - `DISPLAY: power_on wb_io2=14 value=1 mode=input_pullup xram_offset=0`
  - `DISPLAY: spi_bus_check spi_ok=1 tx_calls=25 tx_bytes=35 reset_busy_seen=0`
  - `DISPLAY: white_clear_phase=1`
  - `DISPLAY: white_clear_phase=2`
  - `DISPLAY: white_clear_ok`
  - `APP: result=PASS gate=2 name=display_smoke ... mode=white_clear spi_check=1`
- Visual result:
  - Pending operator confirmation whether edge strip is cleared.

### Gate 2 (`display_smoke`) rerun: SPI-check + `HELLO WORLD` (post-white validation)

- User validation before rerun:
  - White-clear baseline is good (full white achieved).
- Gate 2 mode:
  - `display_service_spi_bus_check()` + `display_service_render_hello_world()`
- Evidence:
  - `APP: heartbeat_skip gate=2 reason=non_heartbeat_gate`
  - `DISPLAY: power_on wb_io2=14 value=1 mode=input_pullup xram_offset=0`
  - `DISPLAY: spi_bus_check spi_ok=1 tx_calls=25 tx_bytes=35 reset_busy_seen=0`
  - `DISPLAY: hello_world_phase=white_clear`
  - `DISPLAY: hello_world_phase=tri_test`
  - `DISPLAY: hello_world_render_ok`
  - `APP: result=PASS gate=2 name=display_smoke display_smoke_ok mode=hello_world spi_check=1`
- Visual result:
  - Pending operator confirmation for visible `HELLO WORLD`.

### Gate Model Update: Gate 2.1 Added

- Change summary:
  - Added dedicated Gate 2.1 (`i2c_smoke`, internal `CONFIG_APP_GATE=21`) so SPI display and I2C bring-up are isolated.
  - Gate 2 remains E-Ink display smoke.
- Pending:
  - First Gate 2.1 execution evidence with selected device mode (`1|2|3`).

## Gate Rerun Session (2026-02-17)

### Gate 1 (`heartbeat`) rerun

- Pin precheck source:
  - `docs/11-pin-mapping-rak3312-rak19007.md`
  - Heartbeat primary: `GPIO46`
- Reusable example:
  - `examples/gates/GATE_1_HEARTBEAT_EXAMPLE.md`
- Config:
  - `CONFIG_APP_GATE=1`
- Evidence:
  - `APP: gate_id_scheme=new selected=1`
  - `APP: boot board=RAK3312 gate=1 gate_raw=1 name=heartbeat ...`
  - `APP: heartbeat_started gpio=46`
  - `APP: gate=1 name=heartbeat heartbeat_toggle_count=5`
  - `APP: result=PASS gate=1 name=heartbeat heartbeat_ok toggles=7`
  - `APP: handshake=STOP_AFTER_PASS gate=1 action=change_CONFIG_APP_GATE_and_reflash`
- Result:
  - Firmware PASS
  - Visual LED confirmation: `YES` (operator confirmed)

### Gate 2 (`display_smoke`) rerun

- Pin precheck source:
  - `docs/11-pin-mapping-rak3312-rak19007.md`
  - Gate 2 profile: `CS12/SCLK13/MOSI11/DC21/BUSY42/PWR14`, `XRAM_OFFSET=0`
- Config:
  - `CONFIG_APP_GATE=2`
- Evidence:
  - `APP: gate_id_scheme=new selected=2`
  - `APP: boot board=RAK3312 gate=2 gate_raw=2 name=display_smoke ...`
  - `DISPLAY: power_on wb_io2=14 value=1 mode=input_pullup xram_offset=0`
  - `DISPLAY: init_ok panel=ssd1680 res=250x122 orientation=landscape native=122x250`
  - `DISPLAY: spi_bus_check spi_ok=1 tx_calls=25 tx_bytes=35 reset_busy_seen=0`
  - `DISPLAY: hello_world_render_ok`
  - `APP: result=PASS gate=2 name=display_smoke display_smoke_ok mode=hello_world spi_check=1`
  - `APP: handshake=STOP_AFTER_PASS gate=2 action=change_CONFIG_APP_GATE_and_reflash`
- Result:
  - Firmware PASS
  - Visible `HELLO WORLD` confirmation: `YES`

### Gate 2 (`display_smoke`) rerun with strong-visibility phases

- Purpose:
  - Replace thin `tri_test` visibility with full-screen `black -> red -> final_card` sequence.
- Config:
  - `CONFIG_APP_GATE=2`
- Evidence:
  - `DISPLAY: hello_world_phase=white_clear`
  - `DISPLAY: hello_world_phase=black_full`
  - `DISPLAY: hello_world_phase=red_full`
  - `DISPLAY: hello_world_phase=final_white_base`
  - `DISPLAY: hello_world_phase=final_text_card`
  - `DISPLAY: hello_world_render_ok`
  - `APP: result=PASS gate=2 name=display_smoke ...`
- Result:
  - Firmware PASS
  - Visual confirmation: color phases visible

### Gate 2 (`display_smoke`) text-visibility rerun (scaled text + base clear)

- Trigger:
  - Operator observed color phases but no readable text.
- Firmware change:
  - Added `final_white_base` phase before final text render.
  - Increased final text size using scaled 5x7 glyph rendering.
- Evidence:
  - `DISPLAY: hello_world_phase=white_clear`
  - `DISPLAY: hello_world_phase=black_full`
  - `DISPLAY: hello_world_phase=red_full`
  - `DISPLAY: hello_world_phase=final_white_base`
  - `DISPLAY: hello_world_phase=final_text_card`
  - `DISPLAY: hello_world_render_ok`
  - `APP: result=PASS gate=2 name=display_smoke ...`
- Result:
  - Firmware PASS
  - Visual text confirmation: `YES` (`HELLO`/`WORLD` visible)
  - Operator evidence: user image attached (2026-02-17)

### Gate 2.1 (`i2c_smoke`) rerun (SGP40-only mode)

- Pin precheck source:
  - `docs/11-pin-mapping-rak3312-rak19007.md`
  - I2C: `SDA=GPIO9`, `SCL=GPIO40`, `3V3` native
- Prompt selection:
  - `1` (SGP40 only)
- Config:
  - `CONFIG_APP_GATE=21`
  - `CONFIG_APP_GATE2P1_EXPECTED_DEVICES=1`
- Evidence:
  - `APP: gate_id_scheme=new selected=21`
  - `APP: boot board=RAK3312 gate=2.1 gate_raw=21 name=i2c_smoke ...`
  - `I2C: init_ok port=0 sda=9 scl=40 freq_hz=100000`
  - `I2C: scan_found addr=0x59`
  - `I2C: scan_done found=1 sgp40=1 bmp280=0`
  - `APP: result=PASS gate=2.1 name=i2c_smoke i2c_smoke_ok alias=2.1 expected=1 sgp40_addr=0x59 bmp280_addr=0x76`
  - `APP: handshake=STOP_AFTER_PASS gate=2.1 action=change_CONFIG_APP_GATE_and_reflash`
- Result:
  - Firmware PASS
  - Expected attached-device mode matched (`1/SGP40 only`)

### Gate 3 (`i2c_presence`) rerun (SGP40-only mode)

- Pin precheck source:
  - `docs/11-pin-mapping-rak3312-rak19007.md`
  - I2C: `SDA=GPIO9`, `SCL=GPIO40`, `3V3` native
- Prompt selection:
  - `1` (SGP40 only)
- Config:
  - `CONFIG_APP_GATE=3`
  - `CONFIG_APP_GATE3_EXPECTED_DEVICES=1`
- Evidence:
  - `APP: gate_id_scheme=new selected=3`
  - `APP: boot board=RAK3312 gate=3 gate_raw=3 name=i2c_presence ...`
  - `I2C: init_ok port=0 sda=9 scl=40 freq_hz=100000`
  - `I2C: scan_found addr=0x59`
  - `I2C: scan_done found=1 sgp40=1 bmp280=0`
  - `APP: result=PASS gate=3 name=i2c_presence presence_ok expected=1 sgp40_addr=0x59 bmp280_addr=0x76`
  - `APP: handshake=STOP_AFTER_PASS gate=3 action=change_CONFIG_APP_GATE_and_reflash`
- Result:
  - Firmware PASS
  - Expected attached-device mode matched (`1/SGP40 only`)

### Gate 4 (`sensor_pipeline`) rerun (SGP40-only, real data path)

- Pin precheck source:
  - `docs/11-pin-mapping-rak3312-rak19007.md`
  - I2C: `SDA=GPIO9`, `SCL=GPIO40`, `3V3` native
- Prompt selection:
  - `1` (SGP40 only)
- Config:
  - `CONFIG_APP_GATE=4`
  - `CONFIG_APP_GATE4_EXPECTED_DEVICES=1`
- Firmware update during rerun:
  - Replaced mock SGP40 VOC output with real I2C measurement command (`0x26 0x0F`) and CRC validation.
  - Added retry logic (`attempt=1..3`) for transient first-read failures.
- Evidence:
  - `APP: gate_id_scheme=new selected=4`
  - `APP: boot board=RAK3312 gate=4 gate_raw=4 name=sensor_pipeline ...`
  - `SENSOR: identity_sgp40 serial_ok=1 serial_words=0000-0640-86FD`
  - `SENSOR: sgp40_read_retry attempt=1 err=ESP_FAIL`
  - `SENSOR: sgp40_data raw=28554 voc_index=106.00`
  - `SENSOR: sgp40_data raw=28560 voc_index=107.00`
  - `APP: result=PASS gate=4 name=sensor_pipeline sensor_pipeline_ok expected=1 identity=1 sensor_ok=3 sensor_fail=0`
  - `APP: handshake=STOP_AFTER_PASS gate=4 action=change_CONFIG_APP_GATE_and_reflash`
- Result:
  - Firmware PASS
  - Real SGP40 data read confirmed (`raw` and mapped `voc_index` logged)

### Gate 5 (`payload_v1`) rerun

- Pin precheck source:
  - `docs/11-pin-mapping-rak3312-rak19007.md`
  - Gate 5 is payload-only; no additional peripheral pin dependency.
- Config:
  - `CONFIG_APP_GATE=5`
- Evidence:
  - `APP: gate_id_scheme=new selected=5`
  - `APP: boot board=RAK3312 gate=5 gate_raw=5 name=payload_v1 ...`
  - `APP: gate=5 payload_schema=1 payload_hex=01 27 10 00 01 8A 24 0B 54 0F A0 01`
  - `APP: result=PASS gate=5 name=payload_v1 payload_encode_ok schema=1 bytes=12`
  - `APP: handshake=STOP_AFTER_PASS gate=5 action=change_CONFIG_APP_GATE_and_reflash`
- Result:
  - Firmware PASS
  - Deterministic payload vector matched expected schema v1 (12 bytes)

### Gate 6 (`lorawan_join_uplink`) rerun

- Pin precheck source:
  - `docs/11-pin-mapping-rak3312-rak19007.md`
  - Core/radio seating unchanged (no additional external pin change for Gate 6).
- Credentials precheck:
  - `firmware/.env` present with `DEVEUI`, `APPKEY`, `JOINEUI`.
  - Secret values intentionally not echoed in logs.
- Config:
  - `CONFIG_APP_GATE=6`
- Evidence:
  - `APP: gate_id_scheme=new selected=6`
  - `APP: boot board=RAK3312 gate=6 gate_raw=6 name=lorawan_join_uplink ...`
  - `LORAWAN: init_stub region=AS923-1 backend_active=1`
  - `LORAWAN: join_start attempt=1`
  - `LORAWAN: join_success attempts=1`
  - `LORAWAN: uplink_stub_ok bytes=12`
  - `APP: result=PASS gate=6 name=lorawan_join_uplink joined=1 uplink_done=1 join_attempts=1 backend_active=1`
  - `APP: handshake=STOP_AFTER_PASS gate=6 action=change_CONFIG_APP_GATE_and_reflash`
- Result:
  - Firmware PASS
  - Join and first uplink control path verified under Gate 6 stop-after-pass policy

### Gate 7 (`reliability_buffer`) rerun

- Pin precheck source:
  - `docs/11-pin-mapping-rak3312-rak19007.md`
  - Core/radio seating unchanged (no additional external pin change for Gate 7).
- Credentials precheck:
  - `firmware/.env` present with `DEVEUI`, `APPKEY`, `JOINEUI`.
  - Secret values intentionally not echoed in logs.
- Config:
  - `CONFIG_APP_GATE=7`
- Evidence:
  - `APP: gate_id_scheme=new selected=7`
  - `APP: boot board=RAK3312 gate=7 gate_raw=7 name=reliability_buffer ...`
  - `LORAWAN: backend_active_set value=0`
  - `APP: gate=7 buffer_store bytes=12`
  - `LORAWAN: backend_active_set value=1`
  - `LORAWAN: join_success attempts=3`
  - `APP: gate=7 buffer_flush_ok flushed_count=1`
  - `APP: result=PASS gate=7 name=reliability_buffer reliability_ok joined=1 buffered=1 flushed=1 uplink_ok=1`
  - `APP: handshake=STOP_AFTER_PASS gate=7 action=change_CONFIG_APP_GATE_and_reflash`
- Result:
  - Firmware PASS
  - Buffer-while-not-joined and flush-after-join control path verified

### Gate 8 (`fuota_scaffold`) rerun

- Pin precheck source:
  - `docs/11-pin-mapping-rak3312-rak19007.md`
  - Core/radio seating unchanged (no additional external pin change for Gate 8).
- Config:
  - `CONFIG_APP_GATE=8`
- Evidence:
  - `APP: gate_id_scheme=new selected=8`
  - `APP: boot board=RAK3312 gate=8 gate_raw=8 name=fuota_scaffold ...`
  - `FUOTA: manifest=placeholder-v1 region=AS923-1 mode=emergency-only`
  - `FUOTA: multicast_hook=ready fragment_hook=ready rollback_policy=hard_required`
  - `FUOTA: version_payload=01 00 00 00 downgrade_block=1 max_downtime_min=30`
  - `APP: result=PASS gate=8 name=fuota_scaffold fuota_scaffold_ready hooks=1 rollback=1`
  - `APP: handshake=STOP_AFTER_PASS gate=8 action=change_CONFIG_APP_GATE_and_reflash`
- Result:
  - Firmware PASS
  - FUOTA scaffold hooks and rollback policy markers verified

### Gate 9 (`live_publish`) rerun (SGP40-only mode)

- Pin precheck source:
  - `docs/11-pin-mapping-rak3312-rak19007.md`
  - Verified Gate 9 dependencies: I2C (`GPIO9/GPIO40`), display SPI/DC/BUSY/PWR profile, core/radio seating unchanged.
- Prompt selection:
  - `1` (SGP40 only)
- Credentials precheck:
  - `firmware/.env` present with `DEVEUI`, `APPKEY`, `JOINEUI`.
  - Secret values intentionally not echoed in logs.
- Config:
  - `CONFIG_APP_GATE=9`
  - `CONFIG_APP_GATE9_EXPECTED_DEVICES=1`
- Evidence:
  - `APP: gate_id_scheme=new selected=9`
  - `APP: boot board=RAK3312 gate=9 gate_raw=9 name=live_publish ...`
  - `SENSOR: sgp40_data raw=31277 voc_index=140.00`
  - `DISPLAY: render_data voc=140.00 pressure=101325.0 temp=25.00 batt=3.95`
  - `LORAWAN: join_success attempts=1`
  - `LORAWAN: uplink_stub_ok bytes=12`
  - `APP: result=PASS gate=9 name=live_publish live_publish_ok expected=1 sensor_ok=2 display_updates=1 uplink_ok=1`
  - `APP: handshake=STOP_AFTER_PASS gate=9 action=change_CONFIG_APP_GATE_and_reflash`
- Result:
  - Firmware PASS
  - End-to-end path verified for selected mode (`sensor -> display -> uplink`)

### Gate 9 (`live_publish`) example-script rerun

- Trigger:
  - Added reusable example artifacts:
    - `examples/gates/GATE_9_LIVE_PUBLISH_EXAMPLE.md`
    - `examples/gates/run_gate_9_live_publish.sh`
- Command:
  - `examples/gates/run_gate_9_live_publish.sh /dev/cu.usbmodem1101 1`
- Evidence:
  - `APP: gate_id_scheme=new selected=9`
  - `SENSOR: sgp40_data raw=31259 voc_index=140.00`
  - `DISPLAY: render_data voc=140.00 pressure=101325.0 temp=25.00 batt=3.95`
  - `LORAWAN: join_success attempts=1`
  - `LORAWAN: uplink_stub_ok bytes=12`
  - `APP: result=PASS gate=9 name=live_publish live_publish_ok expected=1 sensor_ok=2 display_updates=1 uplink_ok=1`
  - `APP: handshake=STOP_AFTER_PASS gate=9 action=change_CONFIG_APP_GATE_and_reflash`
- Result:
  - Firmware PASS
  - Reusable Gate 9 example validated on hardware

### Gate 6 example artifacts added

- Trigger:
  - Requested reusable Gate 6 example and script.
- Added:
  - `examples/gates/GATE_6_LORAWAN_JOIN_UPLINK_EXAMPLE.md`
  - `examples/gates/run_gate_6_lorawan_join_uplink.sh`
- Documentation sync:
  - `examples/gates/README.md`
  - `firmware/README.md`
  - `docs/BRINGUP_PLAN.md`
  - `docs/GATE_RUN_INSTRUCTIONS.md`
  - `docs/USERINSTRUCTION_GATES.md`
- Result:
  - Gate 6 reusable workflow documented and executable with one command.

### Gate 7 (`reliability_buffer`) rerun (post Gate 6 example/doc update)

- Pin precheck source:
  - `docs/11-pin-mapping-rak3312-rak19007.md`
  - Gate 7 dependency confirmed: core/radio seating unchanged.
- Credentials precheck:
  - `firmware/.env` present with `DEVEUI`, `APPKEY`, `JOINEUI`.
  - Secret values intentionally not echoed in logs.
- Config:
  - `CONFIG_APP_GATE=7`
- Evidence:
  - `APP: gate_id_scheme=new selected=7`
  - `APP: boot board=RAK3312 gate=7 gate_raw=7 name=reliability_buffer ...`
  - `LORAWAN: init_stub region=AS923-1 backend_active=1`
  - `LORAWAN: backend_active_set value=0`
  - `APP: gate=7 buffer_store bytes=12`
  - `LORAWAN: backend_active_set value=1`
  - `LORAWAN: join_success attempts=3`
  - `APP: gate=7 buffer_flush_ok flushed_count=1`
  - `APP: result=PASS gate=7 name=reliability_buffer reliability_ok joined=1 buffered=1 flushed=1 uplink_ok=1`
  - `APP: handshake=STOP_AFTER_PASS gate=7 action=change_CONFIG_APP_GATE_and_reflash`
- Result:
  - Firmware PASS
  - Buffered payload was retained pre-join and flushed after join as expected.

### Gate 7 example artifacts added

- Trigger:
  - Requested reusable Gate 7 example and script.
- Added:
  - `examples/gates/GATE_7_RELIABILITY_BUFFER_EXAMPLE.md`
  - `examples/gates/run_gate_7_reliability_buffer.sh`
- Documentation sync:
  - `examples/gates/README.md`
  - `firmware/README.md`
  - `docs/BRINGUP_PLAN.md`
  - `docs/GATE_RUN_INSTRUCTIONS.md`
  - `docs/USERINSTRUCTION_GATES.md`
- Result:
  - Gate 7 reusable workflow documented and executable with one command.

### Gate 8 (`fuota_scaffold`) rerun (post Gate 7 example/doc update)

- Pin precheck source:
  - `docs/11-pin-mapping-rak3312-rak19007.md`
  - Gate 8 dependency confirmed: core/radio seating unchanged.
- Config:
  - `CONFIG_APP_GATE=8`
- Evidence:
  - `APP: gate_id_scheme=new selected=8`
  - `APP: boot board=RAK3312 gate=8 gate_raw=8 name=fuota_scaffold ...`
  - `FUOTA: manifest=placeholder-v1 region=AS923-1 mode=emergency-only`
  - `FUOTA: multicast_hook=ready fragment_hook=ready rollback_policy=hard_required`
  - `FUOTA: version_payload=01 00 00 00 downgrade_block=1 max_downtime_min=30`
  - `APP: result=PASS gate=8 name=fuota_scaffold fuota_scaffold_ready hooks=1 rollback=1`
  - `APP: handshake=STOP_AFTER_PASS gate=8 action=change_CONFIG_APP_GATE_and_reflash`
- Result:
  - Firmware PASS
  - FUOTA scaffold markers validated after Gate 7 example/doc sync.

### Gate 8 example artifacts added

- Trigger:
  - Requested reusable Gate 8 example and script.
- Added:
  - `examples/gates/GATE_8_FUOTA_SCAFFOLD_EXAMPLE.md`
  - `examples/gates/run_gate_8_fuota_scaffold.sh`
- Documentation sync:
  - `examples/gates/README.md`
  - `firmware/README.md`
  - `docs/BRINGUP_PLAN.md`
  - `docs/GATE_RUN_INSTRUCTIONS.md`
  - `docs/USERINSTRUCTION_GATES.md`
- Result:
  - Gate 8 reusable workflow documented and executable with one command.

### Gate 9 (`live_publish`) rerun (post Gate 8 example/doc update, mode `1`)

- Pin precheck source:
  - `docs/11-pin-mapping-rak3312-rak19007.md`
  - Verified Gate 9 dependencies: I2C (`GPIO9/GPIO40`), display SPI/DC/BUSY/PWR profile, core/radio seating unchanged.
- Prompt/selection used:
  - `CONFIG_APP_GATE9_EXPECTED_DEVICES=1` (`SGP40` only).
- Credentials precheck:
  - `firmware/.env` present with `DEVEUI`, `APPKEY`, `JOINEUI`.
  - Secret values intentionally not echoed in logs.
- Config:
  - `CONFIG_APP_GATE=9`
  - `CONFIG_APP_GATE9_EXPECTED_DEVICES=1`
- Evidence:
  - `APP: gate_id_scheme=new selected=9`
  - `APP: boot board=RAK3312 gate=9 gate_raw=9 name=live_publish ...`
  - `SENSOR: sgp40_data raw=31336 voc_index=141.00`
  - `DISPLAY: render_data voc=141.00 pressure=101325.0 temp=25.00 batt=3.95`
  - `LORAWAN: join_success attempts=1`
  - `LORAWAN: uplink_stub_ok bytes=12`
  - `APP: result=PASS gate=9 name=live_publish live_publish_ok expected=1 sensor_ok=2 display_updates=1 uplink_ok=1`
  - `APP: handshake=STOP_AFTER_PASS gate=9 action=change_CONFIG_APP_GATE_and_reflash`
- Result:
  - Firmware PASS
  - End-to-end path revalidated after Gate 8 example/doc sync.
