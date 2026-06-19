# RAK3312 + RAK19007 Pin Mapping (LEGACY — ESP-IDF / ESP32-S3)

> **LEGACY / SUPERSEDED.** This maps the old ESP-IDF "RAK3312" (ESP32-S3) build
> and does **not** apply to the current RAK4630 (nRF52840) PlatformIO firmware.
> For the active pin map see **[11-pin-mapping-rak4630-rak19007.md](11-pin-mapping-rak4630-rak19007.md)**
> (source of truth: `pio/variants/WisCore_RAK4631_Board/variant.h`). Kept for
> historical reference only.

Last updated: 2026-02-16

## Scope

- Core: `RAK3312` (ESP32-S3 + SX1262)
- Base: `RAK19007`
- Display: `RAK14000` (WisBlock IO slot)
- VOC sensor: `RAK12047` (`SGP40`)
- Pressure sensor: external `BMP280` on I2C header

## Core / Bus Baseline

- Heartbeat LED primary: `GPIO46` (`LED_GREEN`)
- Heartbeat LED alternate: `GPIO45` (`LED_BLUE`)
- I2C bus for sensors: `I2C1`
- `I2C1_SDA` -> `GPIO9`
- `I2C1_SCL` -> `GPIO40`

## External BMP280 Wiring (J12)

- `BMP280 VCC` -> `J12 VDD (3V3)`
- `BMP280 GND` -> `J12 GND`
- `BMP280 SCL` -> `J12 SCL` (`GPIO40`)
- `BMP280 SDA` -> `J12 SDA` (`GPIO9`)

## RAK14000 E-Ink Mapping (Current Gate 2 Validation Profile)

- `SPI_CS` -> `GPIO12`
- `SPI_CLK` -> `GPIO13`
- `SPI_MOSI` -> `GPIO11`
- `SPI_MISO` -> `NC` (`-1`, default for RAK14000 tri-color workflow)
- `EPD_DC` -> `WB_IO1` -> `GPIO21`
- `EPD_RST` -> `NC` (`-1`, not used in this profile)
- `EPD_BUSY` -> `WB_IO4` -> `GPIO42`
- `EPD_PWR_EN` -> `WB_IO2` -> `GPIO14`
- `EPD_XRAM_OFFSET` -> `0` (validated clean full-white baseline on this panel)

## Mandatory Display Power Rule

`EPD_PWR_EN` (`WB_IO2`, `GPIO14`) must be asserted before SSD1680 initialization.
For RAK14000 tri-color, use `INPUT_PULLUP` mode on `WB_IO2` (same behavior as official RAK sample).
If power is not enabled first, Gate 2 (`display_smoke`) must FAIL and stop.

Gate 2 probe markers to confirm electrical response:

- `DISPLAY: probe_reset_busy_pulse seen=1` (expected)
- `DISPLAY: probe_refresh_busy_pulse seen=1` (expected)

If BUSY is not wired/reliable on a module revision, fallback timing mode is allowed:

- `DISPLAY: probe_warn no_refresh_busy_pulse_using_time_fallback=1`

## Bring-up Preflight Rule

Before every gate, check this file and verify only the pins used by that gate:

1. Gate 1: heartbeat pins.
2. Gate 2: display SPI + DC/RST/BUSY/PWR pins.
3. Gate 2.1/3/4/9: I2C pins and sensor wiring.
4. Gate 6/7/9: radio/core seating unchanged.

## Sources

- [RAK3312 Overview](https://docs.rakwireless.com/product-categories/wisblock/rak3312/overview/)
- [RAK19007 Overview](https://docs.rakwireless.com/product-categories/wisblock/rak19007/overview/)
- [RAK14000 Overview](https://docs.rakwireless.com/product-categories/wisblock/rak14000/overview/)
