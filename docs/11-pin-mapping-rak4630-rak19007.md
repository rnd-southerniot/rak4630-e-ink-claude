# RAK4630 + RAK19007 Pin Mapping (Source of Truth)

Last updated: 2026-06-19

## Scope

- Core: `RAK4630` (nRF52840 + SX1262), board id `WisCore_RAK4631_Board`
- Base: `RAK19007`
- Display: `RAK14000` (WisBlock IO slot), panel `SSD1680` 250x122
- VOC sensor: `RAK12047` (`SGP40`) at I2C `0x59`
- Pressure sensor: external `BMP280` on I2C header at `0x76`

**Authoritative source files:**
`pio/variants/WisCore_RAK4631_Board/variant.h` (pin numbers) and
`pio/include/board_pins.h` (project aliases). This document mirrors them.

> Arduino pin number == nRF52840 GPIO, 1:1. `P0.00`–`P0.31` are pins `0`–`31`;
> `P1.00`–`P1.15` are pins `32`–`47`.

## Core / Bus Baseline

| Function | Alias | nRF52840 | Arduino pin |
|----------|-------|----------|-------------|
| Heartbeat LED (Green) | `PIN_LED_GREEN` / `LED_BUILTIN` | P1.03 | 35 |
| LED (Blue) | `PIN_LED_BLUE` | P1.04 | 36 |
| I2C SDA (sensors) | `PIN_WIRE_SDA` | P0.13 | 13 |
| I2C SCL (sensors) | `PIN_WIRE_SCL` | P0.14 | 14 |

I2C bus runs at `100 kHz` (`-DAPP_I2C_FREQ_HZ=100000`). LEDs are active HIGH.

## External BMP280 Wiring (I2C header)

- `BMP280 VCC` -> `3V3`
- `BMP280 GND` -> `GND`
- `BMP280 SCL` -> `SCL` (P0.14)
- `BMP280 SDA` -> `SDA` (P0.13)

## RAK14000 E-Ink Mapping (WisBlock IO Slot)

| Signal | Alias | nRF52840 | Arduino pin | Notes |
|--------|-------|----------|-------------|-------|
| `SPI_MOSI` | `PIN_EPD_MOSI` | P0.30 | 30 | IO-slot SPI |
| `SPI_SCLK` | `PIN_EPD_SCLK` | P0.03 | 3 | IO-slot SPI |
| `SPI_CS` | `PIN_EPD_CS` | P0.26 | 26 | IO-slot SPI |
| `EPD_DC` | `PIN_EPD_DC` (`WB_IO1`) | P0.17 | 17 | Data/Command |
| `EPD_BUSY` | `PIN_EPD_BUSY` (`WB_IO4`) | P0.04 | 4 | Busy (active LOW) |
| `EPD_RST` | `PIN_EPD_RST` (`WB_IO5`) | P0.09 | 9 | set `-1` if not wired |
| `EPD_PWR_EN` | `PIN_EPD_PWR` (`WB_IO2`) | P1.02 | 34 | 3V3_S enable (active HIGH) |
| `SPI_MISO` | `PIN_EPD_MISO` | — | -1 | Not used by SSD1680 |

Display SPI clock: `4 MHz` (`-DAPP_DISPLAY_SPI_MHZ=4`). Orientation landscape
(`-DAPP_DISPLAY_LANDSCAPE=1`), panel profile `SSD1680_250X122`.

### Mandatory Display Power Rule

`EPD_PWR_EN` (`WB_IO2`, P1.02) must be asserted before SSD1680 initialization
(`-DAPP_DISPLAY_PWR_ACTIVE_HIGH=1`, `-DAPP_DISPLAY_PWR_INPUT_PULLUP=1`). If power
is not enabled first, Gate 2 (`display_smoke`) must FAIL and stop.

## LoRa SX1262 (internal to RAK4630)

Managed by the SX126x-Arduino library; listed for reference only. Application
code never bit-bangs these — `lora_rak4630_init()` configures them.

| Signal | nRF52840 | Arduino pin |
|--------|----------|-------------|
| NSS | P1.10 | 42 |
| SCLK | P1.11 | 43 |
| MOSI | P1.12 | 44 |
| MISO | P1.13 | 45 |
| BUSY | P1.14 | 46 |
| DIO1 | P1.15 | 47 |
| RESET | P1.06 | 38 |

## Battery ADC

VBAT is read on `PIN_VBAT` (P1.00, Arduino pin 32), enabled via
`-DAPP_BATTERY_ADC_ENABLED=1`. Divider compensation `1.73`, `0.87890625` mV/LSB
(3.6 V ref, 1/6 gain, 14-bit). See `pio/include/board_pins.h`.

## Bring-up Preflight Rule

Before every gate, verify only the pins used by that gate:

1. Gate 1: heartbeat LED (P1.03).
2. Gate 2: display SPI + DC/RST/BUSY/PWR pins.
3. Gate 2.1 / 3 / 4 / 9: I2C pins (P0.13/P0.14) and sensor wiring.
4. Gate 6 / 7 / 9: radio/core seating unchanged; `firmware/.env` credentials present.

## Sources

- [RAK4630 Overview](https://docs.rakwireless.com/product-categories/wisblock/rak4631/overview/)
- [RAK19007 Overview](https://docs.rakwireless.com/product-categories/wisblock/rak19007/overview/)
- [RAK14000 Overview](https://docs.rakwireless.com/product-categories/wisblock/rak14000/overview/)
- Variant: `pio/variants/WisCore_RAK4631_Board/variant.h`
