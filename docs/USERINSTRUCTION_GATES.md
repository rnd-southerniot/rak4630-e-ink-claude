# User Instructions: Execute Gates (Prompt + VSCode)

## Prompt-Driven CLI Workflow

1. Select gate ID scheme (`new` recommended).
2. Set gate ID.
3. Apply gate-specific prompts (`1|2|3` device mode for Gate 2.1/3/4/9; OTAA creds for Gate 6/7/9).
4. Build, flash, monitor.
5. Confirm PASS and stop.
6. Update log/checklist before moving to next gate.

Reusable scripts:

- `/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh`
- `/Users/arif/rak4630-e-ink/examples/gates/set_gate_legacy.sh`
- `/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh`
- `/Users/arif/rak4630-e-ink/examples/gates/run_gate_1_heartbeat.sh`
- `/Users/arif/rak4630-e-ink/examples/gates/run_gate_2_display.sh`
- `/Users/arif/rak4630-e-ink/examples/gates/run_gate_2_1_i2c.sh`
- `/Users/arif/rak4630-e-ink/examples/gates/run_gate_3_i2c_presence.sh`
- `/Users/arif/rak4630-e-ink/examples/gates/run_gate_4_sensor_pipeline.sh`
- `/Users/arif/rak4630-e-ink/examples/gates/run_gate_6_lorawan_join_uplink.sh`
- `/Users/arif/rak4630-e-ink/examples/gates/run_gate_7_reliability_buffer.sh`
- `/Users/arif/rak4630-e-ink/examples/gates/run_gate_8_fuota_scaffold.sh`
- `/Users/arif/rak4630-e-ink/examples/gates/run_gate_9_live_publish.sh`
- `/Users/arif/rak4630-e-ink/examples/gates/GATE_1_HEARTBEAT_EXAMPLE.md`
- `/Users/arif/rak4630-e-ink/examples/gates/GATE_2_DISPLAY_EXAMPLE.md`
- `/Users/arif/rak4630-e-ink/examples/gates/GATE_2_1_I2C_EXAMPLE.md`
- `/Users/arif/rak4630-e-ink/examples/gates/GATE_3_I2C_PRESENCE_EXAMPLE.md`
- `/Users/arif/rak4630-e-ink/examples/gates/GATE_4_SENSOR_PIPELINE_EXAMPLE.md`
- `/Users/arif/rak4630-e-ink/examples/gates/GATE_6_LORAWAN_JOIN_UPLINK_EXAMPLE.md`
- `/Users/arif/rak4630-e-ink/examples/gates/GATE_7_RELIABILITY_BUFFER_EXAMPLE.md`
- `/Users/arif/rak4630-e-ink/examples/gates/GATE_8_FUOTA_SCAFFOLD_EXAMPLE.md`
- `/Users/arif/rak4630-e-ink/examples/gates/GATE_9_LIVE_PUBLISH_EXAMPLE.md`

Example commands:

```bash
# Gate 1 heartbeat
/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh 1
/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 2 display smoke
/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh 2
/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 2.1 i2c smoke, both devices
/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh 2.1 --gate2p1-devices 3
/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 3, SGP40 only
/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh 3 --gate3-devices 1
/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 3 one-command shortcut (SGP40 only)
/Users/arif/rak4630-e-ink/examples/gates/run_gate_3_i2c_presence.sh /dev/cu.usbmodem1101 1

# Gate 4 SGP40 real data read
/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh 4 --gate4-devices 1
/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 4 one-command shortcut (SGP40 only)
/Users/arif/rak4630-e-ink/examples/gates/run_gate_4_sensor_pipeline.sh /dev/cu.usbmodem1101 1

# Gate 6 join + uplink (requires .env)
/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh 6
/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 6 one-command shortcut
/Users/arif/rak4630-e-ink/examples/gates/run_gate_6_lorawan_join_uplink.sh /dev/cu.usbmodem1101

# Gate 7 reliability buffer (requires .env)
/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh 7
/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 7 one-command shortcut
/Users/arif/rak4630-e-ink/examples/gates/run_gate_7_reliability_buffer.sh /dev/cu.usbmodem1101

# Gate 8 FUOTA scaffold
/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh 8
/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 8 one-command shortcut
/Users/arif/rak4630-e-ink/examples/gates/run_gate_8_fuota_scaffold.sh /dev/cu.usbmodem1101

# Gate 9, both sensors
/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh 9 --gate9-devices 3
/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 9 one-command shortcut (SGP40 only)
/Users/arif/rak4630-e-ink/examples/gates/run_gate_9_live_publish.sh /dev/cu.usbmodem1101 1
```

## VSCode IDE Workflow

Use tasks in `/Users/arif/rak4630-e-ink/.vscode/tasks.json`:

- `Gate: Set New ID`
- `Gate: Set Legacy ID`
- `Gate: Build`
- `Gate: Flash+Monitor`
- `Gate: Run Gate 1 Heartbeat`
- `Gate: Run Gate 2 Display Smoke`
- `Gate: Run Gate 2.1 I2C Smoke`

Use launch profile in `/Users/arif/rak4630-e-ink/.vscode/launch.json`:

- `ESP-IDF Monitor (Gate Tags)`

This monitor applies tag filtering for `APP`, `DISPLAY`, `SENSOR`, `LORAWAN`, `FUOTA`, `I2C`.

## Prompt Rules by Gate

- Gate 1: heartbeat-only gate, no sensor prompt.
  - Verify pin mapping first (`GPIO46` primary).
  - Expected markers include `APP: heartbeat_started gpio=46` and `APP: result=PASS gate=1 ...`.
- Gate 2: display-only smoke test, no sensor prompt.
  - Use validated tri-color baseline: `CONFIG_APP_DISPLAY_XRAM_OFFSET=0`, `CONFIG_APP_DISPLAY_PWR_INPUT_PULLUP=y`.
  - Expected markers include `DISPLAY: spi_bus_check spi_ok=1 ...` and `DISPLAY: hello_world_render_ok`.
- Gate 2.1: I2C-only smoke gate. Ask connected I2C devices (`1|2|3`) and set `CONFIG_APP_GATE2P1_EXPECTED_DEVICES`.
- Gate 3: ask connected I2C devices (`1|2|3`).
- Gate 4: ask selected sensor mode (`1|2|3`).
- Gate 6/7/9: always ask for `DEVEUI` and `APPKEY` in `firmware/.env`, keep `JOINEUI=0000000000000000` default.
- After Gate 8 PASS: ask Gate 9 mode (`1|2|3`) and set `CONFIG_APP_GATE9_EXPECTED_DEVICES`.
