# Troubleshooting Skills (Reusable SOP)

## Skill 1: Gate 1 Heartbeat No Blink

- Check pin mapping doc first.
- Verify `CONFIG_APP_HEARTBEAT_GPIO=46` (fallback `45`).
- Re-run Gate 1 and require visual confirmation.
- PASS marker: `APP: result=PASS gate=1 name=heartbeat ...`

## Skill 2: Gate 2 Display Smoke Fail

- Check display pin mapping (`CS/CLK/MOSI/MISO/DC/RST/BUSY/PWR`).
- Use RAK tri-color baseline:
  - `CS=12`, `SCLK=13`, `MOSI=11`, `MISO=-1`
  - `DC=21 (WB_IO1)`, `RST=-1`, `BUSY=42 (WB_IO4)`, `PWR=14 (WB_IO2)`
  - `CONFIG_APP_DISPLAY_XRAM_OFFSET=0`
  - `CONFIG_APP_DISPLAY_PWR_INPUT_PULLUP=y`
- Confirm display power enable (`EPD_PWR_EN` / `GPIO14`) is in `input_pullup` mode before init.
- Re-run Gate 2 and verify:
  - `DISPLAY: power_on wb_io2=... mode=input_pullup xram_offset=0`
  - `DISPLAY: spi_bus_check spi_ok=1 ...`
  - `DISPLAY: init_ok panel=ssd1680 res=...`
  - `DISPLAY: hello_world_phase=black_full`
  - `DISPLAY: hello_world_phase=red_full`
  - `DISPLAY: hello_world_phase=final_white_base`
  - `DISPLAY: hello_world_phase=final_text_card`
  - `DISPLAY: hello_world_render_ok`
- PASS marker: `APP: result=PASS gate=2 name=display_smoke ...`

## Skill 3: Gate 3 I2C Presence Mismatch

- Ask selected mode (`1|2|3`) and set `CONFIG_APP_GATE3_EXPECTED_DEVICES`.
- Re-check SGP40 slot seating and BMP280 wiring on `J12`.
- Re-run Gate 3 and validate `I2C: scan_found` addresses.

## Skill 3A: Gate 2.1 I2C Smoke Fail

- Ask selected mode (`1|2|3`) and set `CONFIG_APP_GATE2P1_EXPECTED_DEVICES`.
- Re-check only I2C physical path first (`SDA/SCL/GND/3V3`) before sensor-pipeline debugging.
- Re-run Gate 2.1 and verify:
  - `I2C: scan_found ...`
  - `APP: result=PASS gate=2.1 name=i2c_smoke ...`

## Skill 4: Gate 6/7 Join or Buffer Issues

- Confirm `.env` has per-device `DEVEUI` and `APPKEY`.
- Keep `JOINEUI=0000000000000000` unless instructed otherwise.
- For Gate 7 confirm sequence:
  - `buffer_store`
  - backend enable
  - `buffer_flush_ok`

## Skill 5: Gate 9 Stuck With No Display/Uplink

- After Gate 8 PASS, verify Gate 9 mode selection (`1|2|3`).
- Confirm `CONFIG_APP_GATE9_EXPECTED_DEVICES` matches hardware.
- Ensure threshold/min-refresh settings are not blocking first render.
- PASS marker: `APP: result=PASS gate=9 name=live_publish ...`

## Skill 6: Gate 2 PASS Logs but No Visible Image

- Symptom: `hello_world_render_ok` appears but panel still looks blank.
- Checks:
  1. Verify BUSY polarity (`APP_DISPLAY_BUSY_ACTIVE_HIGH`, default `n` for SSD1680).
  2. Verify native panel geometry mapping is active (`native=122x250` for 250x122 profile).
  3. Reflash Gate 2 and observe full marker sequence.
  4. Confirm visible result with operator before proceeding.
- PASS signature:
  - Firmware PASS markers present and operator confirms visible black/red cycle and final card text.

## Skill 7: Gate 2 Noisy/Static Red-White Screen

- Symptom: panel powers but screen shows random red/white static.
- Actions:
  1. Keep Gate 2 only (`CONFIG_APP_GATE=2`) and stop-after-pass policy.
  2. Enforce tri-color config baseline from Skill 2.
  3. Verify these logs appear after flash:
     - `DISPLAY: power_on wb_io2=14 value=1 mode=input_pullup xram_offset=0`
     - `DISPLAY: hello_world_phase=white_clear`
     - `DISPLAY: hello_world_phase=black_full`
     - `DISPLAY: hello_world_phase=red_full`
     - `DISPLAY: hello_world_phase=final_white_base`
     - `DISPLAY: hello_world_phase=final_text_card`
  4. If `probe_warn no_refresh_busy_pulse_using_time_fallback=1` appears, allow fallback and wait full refresh time.
  5. Require user visual confirmation before moving to Gate 3.
