---
name: rak4630-pio-build
description: Build, flash, and monitor the RAK4630 PlatformIO firmware. Select gates, configure build flags, and manage the dev cycle.
---

# RAK4630 PlatformIO Build Workflow

## Build

```bash
cd pio/
~/.platformio/penv/bin/pio run
```

Clean build:
```bash
~/.platformio/penv/bin/pio run -t clean && ~/.platformio/penv/bin/pio run
```

## Flash + Monitor

```bash
~/.platformio/penv/bin/pio run -t upload && ~/.platformio/penv/bin/pio device monitor -b 115200
```

Or combined:
```bash
~/.platformio/penv/bin/pio run -t upload -t monitor
```

## Select a Gate

Edit `pio/platformio.ini` and change the `-DAPP_GATE=` value:

| Flag Value | Gate | What it validates |
|-----------|------|-------------------|
| 0 | env | Toolchain sanity (instant pass) |
| 1 | heartbeat | LED toggle |
| 2 | display_smoke | SPI + SSD1680 render |
| 21 | i2c_smoke | I2C bus scan |
| 3 | i2c_presence | SGP40/BMP280 address probe |
| 4 | sensor_pipeline | Sensor read + plausibility |
| 5 | payload_v1 | Payload encode test |
| 6 | lorawan_join_uplink | OTAA join + uplink (stub) |
| 7 | reliability_buffer | Buffer flush (stub) |
| 8 | fuota_scaffold | FUOTA placeholder |
| 9 | live_publish | Full cycle |

Example: To run Gate 2 (display smoke test):
```ini
build_flags =
    -DAPP_GATE=2
    ...
```

Then rebuild and flash.

## Device Expectations

For gates that probe I2C devices, set the expected device flag:

```ini
-DAPP_GATE2P1_EXPECTED_DEVICES=3   # Gate 21: 1=SGP40, 2=BMP280, 3=both
-DAPP_GATE3_EXPECTED_DEVICES=1     # Gate 3
-DAPP_GATE4_EXPECTED_DEVICES=1     # Gate 4
-DAPP_GATE9_EXPECTED_DEVICES=1     # Gate 9
```

## Key Build Flags

All configuration is via `-D` flags in `platformio.ini` (no Kconfig/menuconfig):

| Flag | Default | Description |
|------|---------|-------------|
| `APP_GATE` | 0 | Which gate to run |
| `APP_GATE_TICK_MS` | 200 | Gate runner tick period |
| `APP_I2C_FREQ_HZ` | 100000 | I2C bus frequency |
| `APP_DISPLAY_SPI_MHZ` | 4 | E-Ink SPI clock |
| `APP_DISPLAY_LANDSCAPE` | 1 | Landscape orientation |
| `APP_DISPLAY_MIN_REFRESH_SEC` | 30 | Min seconds between refreshes |
| `APP_LORAWAN_BACKEND_ACTIVE` | 1 | Enable LoRaWAN backend |
| `APP_BATTERY_ADC_ENABLED` | 1 | Enable battery voltage ADC |

## Host Tests

Host-side tests (no hardware) still work from the repo root:

```bash
tests/host/run_tests.sh
```

## Project Structure

```
pio/
  platformio.ini          # Build config + all -D flags
  boards/                 # Custom board JSON (wiscore_rak4631.json)
  variants/               # nRF52840 variant (pin mapping)
  include/                # Headers (board_pins.h, compat.h, ...)
  src/                    # Source files (.cpp + .c)
  .pio/                   # Build output (gitignored)
```
