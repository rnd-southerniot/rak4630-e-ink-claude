# Gate Test Examples (Reusable)

This folder contains reusable gate test scripts.

## Files

- `set_gate_new.sh`: set canonical gate `0..9`, plus `2.1` alias (`21`) for I2C smoke.
- `set_gate_legacy.sh`: set legacy gate `0..8` with compatibility mapping.
- `run_gate.sh`: export ESP-IDF, build, flash, monitor.
- `run_gate_1_heartbeat.sh`: one-command Gate 1 (heartbeat).
- `run_gate_2_display.sh`: one-command Gate 2 (E-Ink display smoke).
- `run_gate_2_1_i2c.sh`: one-command Gate 2.1 (I2C smoke).
- `run_gate_3_i2c_presence.sh`: one-command Gate 3 (I2C presence).
- `run_gate_4_sensor_pipeline.sh`: one-command Gate 4 (sensor pipeline).
- `run_gate_6_lorawan_join_uplink.sh`: one-command Gate 6 (LoRaWAN join/uplink).
- `run_gate_7_reliability_buffer.sh`: one-command Gate 7 (reliability buffer).
- `run_gate_8_fuota_scaffold.sh`: one-command Gate 8 (FUOTA scaffold).
- `run_gate_9_live_publish.sh`: one-command Gate 9 (live publish).
- `GATE_1_HEARTBEAT_EXAMPLE.md`: Gate 1 step-by-step example with expected logs.
- `GATE_2_DISPLAY_EXAMPLE.md`: Gate 2 step-by-step example with expected logs.
- `GATE_2_1_I2C_EXAMPLE.md`: Gate 2.1 step-by-step example with expected logs.
- `GATE_3_I2C_PRESENCE_EXAMPLE.md`: Gate 3 step-by-step example with expected logs.
- `GATE_4_SENSOR_PIPELINE_EXAMPLE.md`: Gate 4 step-by-step example with expected logs.
- `GATE_6_LORAWAN_JOIN_UPLINK_EXAMPLE.md`: Gate 6 step-by-step example with expected logs.
- `GATE_7_RELIABILITY_BUFFER_EXAMPLE.md`: Gate 7 step-by-step example with expected logs.
- `GATE_8_FUOTA_SCAFFOLD_EXAMPLE.md`: Gate 8 step-by-step example with expected logs.
- `GATE_9_LIVE_PUBLISH_EXAMPLE.md`: Gate 9 step-by-step example with expected logs.

## Canonical Examples

```bash
# Gate 1 heartbeat
examples/gates/set_gate_new.sh 1
examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 2 display smoke (Hello World)
examples/gates/set_gate_new.sh 2
examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 2.1 i2c smoke, both devices
examples/gates/set_gate_new.sh 2.1 --gate2p1-devices 3
examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 3 i2c presence, SGP40 only
examples/gates/set_gate_new.sh 3 --gate3-devices 1
examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 4 sensor pipeline, both sensors
examples/gates/set_gate_new.sh 4 --gate4-devices 3
examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 5 payload
examples/gates/set_gate_new.sh 5
examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 6 join + uplink (requires .env)
examples/gates/set_gate_new.sh 6
examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 7 reliability buffer (requires .env)
examples/gates/set_gate_new.sh 7
examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 8 fuota scaffold
examples/gates/set_gate_new.sh 8
examples/gates/run_gate.sh /dev/cu.usbmodem1101

# Gate 9 live publish, selected SGP40 only (requires .env)
examples/gates/set_gate_new.sh 9 --gate9-devices 1
examples/gates/run_gate.sh /dev/cu.usbmodem1101
```

## Legacy Example

```bash
examples/gates/set_gate_legacy.sh 2
examples/gates/run_gate.sh /dev/cu.usbmodem1101
```

Legacy `2` resolves to canonical `3`.

## Quick One-Command Gate Runs

```bash
# Gate 1 heartbeat
examples/gates/run_gate_1_heartbeat.sh /dev/cu.usbmodem1101

# Gate 2 E-Ink smoke
examples/gates/run_gate_2_display.sh /dev/cu.usbmodem1101

# Gate 2.1 I2C smoke (devices: 1=SGP40, 2=BMP280, 3=both)
examples/gates/run_gate_2_1_i2c.sh /dev/cu.usbmodem1101 3

# Gate 3 I2C presence (devices: 1=SGP40, 2=BMP280, 3=both)
examples/gates/run_gate_3_i2c_presence.sh /dev/cu.usbmodem1101 1

# Gate 4 sensor pipeline (devices: 1=SGP40, 2=BMP280, 3=both)
examples/gates/run_gate_4_sensor_pipeline.sh /dev/cu.usbmodem1101 1

# Gate 6 LoRaWAN join/uplink
examples/gates/run_gate_6_lorawan_join_uplink.sh /dev/cu.usbmodem1101

# Gate 7 reliability buffer
examples/gates/run_gate_7_reliability_buffer.sh /dev/cu.usbmodem1101

# Gate 8 FUOTA scaffold
examples/gates/run_gate_8_fuota_scaffold.sh /dev/cu.usbmodem1101

# Gate 9 live publish (devices: 1=SGP40, 2=BMP280, 3=both)
examples/gates/run_gate_9_live_publish.sh /dev/cu.usbmodem1101 1
```
