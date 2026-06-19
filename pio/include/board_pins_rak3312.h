#pragma once

/*
 * RAK3312 (ESP32-S3 + SX1262) WisBlock pin mapping on RAK19007.
 * Raw ESP32-S3 GPIO numbers.
 * Sources: RAKwireless RAK3312 datasheet (SX1262 / I2C1 / LEDs) and the legacy
 * firmware/sdkconfig.defaults + docs/11-pin-mapping-rak3312-rak19007.md (display).
 */

/* --- I2C (WisBlock Sensor Slot, I2C1) --- */
#define PIN_I2C_SDA         9
#define PIN_I2C_SCL         40

/* --- Built-in LEDs --- */
#define PIN_LED_GREEN       46
#define PIN_LED_BLUE        45
#define PIN_HEARTBEAT_LED   PIN_LED_GREEN

/* --- RAK14000 E-Ink Display (WisBlock IO Slot, SSD1680) --- */
#define PIN_EPD_MOSI        11              /* IO slot SPI */
#define PIN_EPD_MISO        (-1)            /* Not used by SSD1680 */
#define PIN_EPD_SCLK        13              /* IO slot SPI */
#define PIN_EPD_CS          12              /* IO slot SPI */
#define PIN_EPD_DC          21              /* WB_IO1 — Data/Command */
#define PIN_EPD_BUSY        42              /* WB_IO4 — Display busy */
#define PIN_EPD_RST         (-1)            /* not wired */
#define PIN_EPD_PWR         14              /* WB_IO2 — 3V3_S power enable */

/* --- Sensor/IO-slot power rail (3V3_S, same WB_IO2 net as the display) --- */
#define PIN_SENSOR_PWR      14              /* WB_IO2 */

/* --- SX1262 LoRa (on-module, ESP32-S3) — used by src/board/rak3312 radio init --- */
#define PIN_LORA_NSS_RAK3312    7
#define PIN_LORA_SCLK_RAK3312   5
#define PIN_LORA_MOSI_RAK3312   6
#define PIN_LORA_MISO_RAK3312   3
#define PIN_LORA_RESET_RAK3312  8
#define PIN_LORA_DIO1_RAK3312   47
#define PIN_LORA_BUSY_RAK3312   48
#define PIN_LORA_ANTSW_RAK3312  4

/* --- Battery ADC (ESP32-S3 VBAT GPIO unconfirmed; disabled via APP_BATTERY_ADC_ENABLED=0) --- */
#define PIN_BATTERY_VBAT    (-1)            /* TBD — confirm and wire later */
#define VBAT_DIVIDER_COMP   1.73f
#define VBAT_MV_PER_LSB_F   (0.80566406f)  /* placeholder; recompute when wired */
