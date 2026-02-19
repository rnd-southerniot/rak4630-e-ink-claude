---
name: rak4630-pin-reference
description: Quick pin reference for RAK4630 (nRF52840) on RAK19007 base board with RAK14000 E-Ink display.
---

# RAK4630 Pin Reference

## nRF52840 GPIO Numbering

Arduino pin N = nRF52840 GPIO N (1:1 mapping via g_ADigitalPinMap).
- P0.00-P0.31 = GPIO 0-31 = Arduino pins 0-31
- P1.00-P1.15 = GPIO 32-47 = Arduino pins 32-47

## I2C Bus (Sensor Slots)

| Bus | SDA | SCL | Macro |
|-----|-----|-----|-------|
| Primary (Wire) | pin 13 / P0.13 | pin 14 / P0.14 | `PIN_WIRE_SDA`, `PIN_WIRE_SCL` |
| Secondary (Wire1) | pin 24 / P0.24 | pin 25 / P0.25 | `PIN_WIRE1_SDA`, `PIN_WIRE1_SCL` |

SGP40 at 0x59, BMP280 at 0x76 — both on primary I2C bus.

## IO Slot SPI (RAK14000 E-Ink)

| Signal | Pin | GPIO | Macro |
|--------|-----|------|-------|
| MOSI | 30 | P0.30 | `MOSI` / `WB_SPI_MOSI` |
| MISO | 29 | P0.29 | `MISO` / `WB_SPI_MISO` |
| SCK | 3 | P0.03 | `SCK` / `WB_SPI_CLK` |
| CS | 26 | P0.26 | `SS` / `WB_SPI_CS` |

## SX1262 LoRa SPI (Internal — managed by library)

| Signal | Pin | GPIO | Macro |
|--------|-----|------|-------|
| NSS | 42 | P1.10 | `RAK_LORA_NSS` |
| SCLK | 43 | P1.11 | `RAK_LORA_SCLK` |
| MOSI | 44 | P1.12 | `RAK_LORA_MOSI` |
| MISO | 45 | P1.13 | `RAK_LORA_MISO` |
| RESET | 38 | P1.06 | `RAK_LORA_RESET` |
| DIO_1 | 47 | P1.15 | `RAK_LORA_DIO_1` |
| BUSY | 46 | P1.14 | `RAK_LORA_BUSY` |

Do NOT use these as default SPI — they conflict with the LoRa transceiver.

## RAK14000 E-Ink Display Control

| Signal | Pin | GPIO | Macro | Notes |
|--------|-----|------|-------|-------|
| DC | 17 | P0.17 | `WB_IO1` | Data/Command select |
| PWR | 34 | P1.02 | `WB_IO2` | 3V3_S power enable (HIGH to enable) |
| BUSY | 4 | P0.04 | `WB_IO4` | Display busy signal |
| RST | 9 | P0.09 | `WB_IO5` | Reset (set -1 if not wired) |

## LEDs

| LED | Pin | GPIO | Macro |
|-----|-----|------|-------|
| Green | 35 | P1.03 | `LED_GREEN` / `PIN_LED1` / `LED_BUILTIN` |
| Blue | 36 | P1.04 | `LED_BLUE` / `PIN_LED2` |

Active HIGH (`LED_STATE_ON=1`).

## WisBlock IO Slot GPIO

| Macro | Pin | GPIO | Used For |
|-------|-----|------|----------|
| `WB_IO1` | 17 | P0.17 | EPD DC |
| `WB_IO2` | 34 | P1.02 | 3V3_S power enable |
| `WB_IO3` | 21 | P0.21 | RAK14000 button S1 |
| `WB_IO4` | 4 | P0.04 | EPD BUSY |
| `WB_IO5` | 9 | P0.09 | RAK14000 button S2 / EPD RST |
| `WB_IO6` | 10 | P0.10 | RAK14000 button S3 |
| `WB_SW1` | 33 | P1.01 | Base board user button |

## Battery ADC

- VBAT pin: 32 (P1.00) via `PIN_VBAT`
- 14-bit ADC, 3.6V reference, 1/6 gain
- Voltage divider compensation: 1.73x

## Source of Truth

- Variant definition: `pio/variants/WisCore_RAK4631_Board/variant.h`
- Pin aliases: `pio/include/board_pins.h`
