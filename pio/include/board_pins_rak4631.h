#pragma once

/*
 * RAK4630 (nRF52840) WisBlock pin mapping
 * Base board: RAK19007
 *
 * These are Arduino pin numbers for the RAKwireless BSP.
 * Source of truth: RAK4631 variant.h in the BSP and RAK14000 datasheet.
 */

/* --- I2C (WisBlock Sensor Slot) --- */
#define PIN_I2C_SDA         PIN_WIRE_SDA    /* P0.13 */
#define PIN_I2C_SCL         PIN_WIRE_SCL    /* P0.14 */

/* --- Built-in LEDs --- */
#define PIN_LED_GREEN       LED_GREEN       /* P1.03 = pin 35 */
#define PIN_LED_BLUE        LED_BLUE        /* P1.04 = pin 36 */
#define PIN_HEARTBEAT_LED   PIN_LED_GREEN

/* --- RAK14000 E-Ink Display (WisBlock IO Slot) --- */
/* RAK14000 on RAK19007 IO slot mapping for nRF52840 */
#define PIN_EPD_MOSI        MOSI            /* P0.30 — IO slot SPI */
#define PIN_EPD_MISO        (-1)            /* Not used by SSD1680 */
#define PIN_EPD_SCLK        SCK             /* P0.03 — IO slot SPI */
#define PIN_EPD_CS          SS              /* P0.26 — IO slot SPI */
#define PIN_EPD_DC          WB_IO1          /* P0.17 — Data/Command */
#define PIN_EPD_BUSY        WB_IO4          /* P0.04 — Display busy */
#define PIN_EPD_RST         WB_IO5          /* P0.09 — set -1 if not wired */
#define PIN_EPD_PWR         WB_IO2          /* P1.02 — 3V3_S power enable */

/* --- Sensor/IO-slot power rail (3V3_S, same WB_IO2 net as the display) --- */
#define PIN_SENSOR_PWR      WB_IO2          /* P1.02 */

/* --- Battery ADC --- */
/* nRF52840 VBAT measurement: the RAK4631 routes the battery divider to WB_A0
 * (P0.05 / AIN3). The variant's PIN_VBAT (P1.00) has NO ADC channel and reads 0.
 * Source of truth: RAKwireless WisBlock RAK4630_Battery_Level_Detect example
 * (#define PIN_VBAT WB_A0). */
#define PIN_BATTERY_VBAT    WB_A0           /* P0.05 / AIN3 */

/* Battery ADC constants for nRF52840 (3.6V reference, 1/6 gain, 14-bit) */
#define VBAT_DIVIDER_COMP   1.73f           /* Compensation for voltage divider */
#define VBAT_MV_PER_LSB_F   (0.87890625f)   /* 3.6V ref / 4096 * 1000 */
