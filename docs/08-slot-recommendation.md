# Slot and Wiring Recommendation

## Recommended Assembly

- Core socket: `RAK3312` (ESP32-S3 + SX1262)
- IO slot: `RAK14000` E-Ink
- Slot A: `RAK12047` (SGP40 VOC)
- External I2C header: `BMP280` breakout

## Why This Mapping

- `RAK14000` requires IO-slot resources.
- `RAK3312` includes LoRa in the core, so no separate LPWAN IO module is needed.
- `RAK12047` and `BMP280` share I2C safely in standard configuration.

## External BMP280 Wiring

- `VCC` -> `3V3`
- `GND` -> `GND`
- `SDA` -> `SDA`
- `SCL` -> `SCL`

## Bring-up Checks

1. Boot with I2C scan enabled.
2. Confirm both sensor addresses are detected.
3. Verify sensor reads before enabling LoRa task.
