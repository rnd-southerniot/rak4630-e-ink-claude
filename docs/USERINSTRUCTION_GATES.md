# User Instructions: Execute Gates (Prompt + VSCode)

## Prompt-Driven CLI Workflow

1. Set the gate via `-DAPP_GATE=N` in `pio/platformio.ini` (`N` in `0..9`, or `21` for Gate 2.1).
2. Apply gate-specific prompts (`1|2|3` device mode for Gate 2.1/3/4/9 via `-DAPP_GATE*_EXPECTED_DEVICES`; OTAA creds for Gate 6/7/9 in `firmware/.env`).
3. Build, flash, monitor (PlatformIO, from `pio/`, monitor baud `115200`).
4. Confirm PASS and stop.
5. Update log/checklist before moving to next gate.

Reusable scripts:

- `examples/gates/set_gate_new.sh`
- `examples/gates/set_gate_legacy.sh`
- `examples/gates/run_gate.sh`
- `examples/gates/run_gate_1_heartbeat.sh`
- `examples/gates/run_gate_2_display.sh`
- `examples/gates/run_gate_2_1_i2c.sh`
- `examples/gates/run_gate_3_i2c_presence.sh`
- `examples/gates/run_gate_4_sensor_pipeline.sh`
- `examples/gates/run_gate_6_lorawan_join_uplink.sh`
- `examples/gates/run_gate_7_reliability_buffer.sh`
- `examples/gates/run_gate_8_fuota_scaffold.sh`
- `examples/gates/run_gate_9_live_publish.sh`
- `examples/gates/GATE_1_HEARTBEAT_EXAMPLE.md`
- `examples/gates/GATE_2_DISPLAY_EXAMPLE.md`
- `examples/gates/GATE_2_1_I2C_EXAMPLE.md`
- `examples/gates/GATE_3_I2C_PRESENCE_EXAMPLE.md`
- `examples/gates/GATE_4_SENSOR_PIPELINE_EXAMPLE.md`
- `examples/gates/GATE_6_LORAWAN_JOIN_UPLINK_EXAMPLE.md`
- `examples/gates/GATE_7_RELIABILITY_BUFFER_EXAMPLE.md`
- `examples/gates/GATE_8_FUOTA_SCAFFOLD_EXAMPLE.md`
- `examples/gates/GATE_9_LIVE_PUBLISH_EXAMPLE.md`

Example commands:

```bash
# Gate 1 heartbeat
examples/gates/set_gate_new.sh 1
examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 2 display smoke
examples/gates/set_gate_new.sh 2
examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 2.1 i2c smoke, both devices
examples/gates/set_gate_new.sh 2.1 --gate2p1-devices 3
examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 3, SGP40 only
examples/gates/set_gate_new.sh 3 --gate3-devices 1
examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 3 one-command shortcut (SGP40 only)
examples/gates/run_gate_3_i2c_presence.sh /dev/cu.usbmodem1101 1

# Gate 4 SGP40 real data read
examples/gates/set_gate_new.sh 4 --gate4-devices 1
examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 4 one-command shortcut (SGP40 only)
examples/gates/run_gate_4_sensor_pipeline.sh /dev/cu.usbmodem1101 1

# Gate 6 join + uplink (requires .env)
examples/gates/set_gate_new.sh 6
examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 6 one-command shortcut
examples/gates/run_gate_6_lorawan_join_uplink.sh /dev/cu.usbmodem1101

# Gate 7 reliability buffer (requires .env)
examples/gates/set_gate_new.sh 7
examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 7 one-command shortcut
examples/gates/run_gate_7_reliability_buffer.sh /dev/cu.usbmodem1101

# Gate 8 FUOTA scaffold
examples/gates/set_gate_new.sh 8
examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 8 one-command shortcut
examples/gates/run_gate_8_fuota_scaffold.sh /dev/cu.usbmodem1101

# Gate 9, both sensors
examples/gates/set_gate_new.sh 9 --gate9-devices 3
examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 9 one-command shortcut (SGP40 only)
examples/gates/run_gate_9_live_publish.sh /dev/cu.usbmodem1101 1
```

## VSCode IDE Workflow

Use the PlatformIO IDE extension for VSCode. The PlatformIO toolbar / project tasks provide:

- `Build` (`pio run`)
- `Upload` (`pio run -t upload`)
- `Monitor` (`pio device monitor`, baud `115200`)
- `Upload and Monitor` (`pio run -t upload -t monitor`)
- `Clean` (`pio run -t clean`)

Gate selection is done by editing `-DAPP_GATE=N` in `pio/platformio.ini`, then rebuilding.

The serial monitor surfaces tag-prefixed lines for `APP`, `DISPLAY`, `SENSOR`, `LORAWAN`, `FUOTA`, `I2C`.

## Prompt Rules by Gate

- Gate 1: heartbeat-only gate, no sensor prompt.
  - Verify pin mapping first (`P1.03` Green LED primary).
  - Expected markers include `APP: heartbeat_started pin=P1.03` and `APP: result=PASS gate=1 ...`.
- Gate 2: display-only smoke test, no sensor prompt.
  - Display SPI pins (IO slot): MOSI=P0.30, SCK=P0.03, CS=P0.26, DC=P0.17, BUSY=P0.04, PWR=P1.02.
  - Expected markers include `DISPLAY: spi_bus_check spi_ok=1 ...` and `DISPLAY: hello_world_render_ok`.
- Gate 2.1: I2C-only smoke gate (SDA=P0.13, SCL=P0.14). Ask connected I2C devices (`1|2|3`) and set `-DAPP_GATE2P1_EXPECTED_DEVICES`.
- Gate 3: ask connected I2C devices (`1|2|3`); set `-DAPP_GATE3_EXPECTED_DEVICES`.
- Gate 4: ask selected sensor mode (`1|2|3`); set `-DAPP_GATE4_EXPECTED_DEVICES`.
- Gate 6/7/9: always ask for `DEVEUI` and `APPKEY` in `firmware/.env`, keep `JOINEUI=0000000000000000` default. Real SX126x-Arduino OTAA, AS923-1, ChirpStack v4, CONFIRMED uplinks.
- After Gate 8 PASS: ask Gate 9 mode (`1|2|3`) and set `-DAPP_GATE9_EXPECTED_DEVICES`.
