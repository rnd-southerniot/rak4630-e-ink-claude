/*
 * RAK4631 WisBlock Core variant pin definitions
 * Based on RAKwireless BSP for Adafruit nRF52 Arduino core
 */

#ifndef _VARIANT_RAK4631_
#define _VARIANT_RAK4631_

/** Master clock frequency */
#define VARIANT_MCK       (64000000ul)

#define USE_LFXO    /* Board uses LFXO crystal for LF clock */

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "WVariant.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Number of pins defined in PinDescription array */
#define PINS_COUNT           (48)
#define NUM_DIGITAL_PINS     (48)
#define NUM_ANALOG_INPUTS    (6)
#define NUM_ANALOG_OUTPUTS   (0)

/* --- LEDs --- */
#define PIN_LED1        (35)  /* P1.03 Green LED */
#define PIN_LED2        (36)  /* P1.04 Blue LED */

#define LED_BUILTIN     PIN_LED1
#define LED_GREEN       PIN_LED1
#define LED_BLUE        PIN_LED2
#define LED_STATE_ON    1

/* --- Analog --- */
#define PIN_A0          (5)   /* P0.05 / AIN3 */
#define PIN_A1          (31)  /* P0.31 / AIN7 */
#define PIN_A2          (28)  /* P0.28 / AIN4 */
#define PIN_A3          (29)  /* P0.29 / AIN5 */
#define PIN_A4          (30)  /* P0.30 / AIN6 */
#define PIN_A5          (31)  /* P0.31 / AIN7 */

static const uint8_t A0 = PIN_A0;
static const uint8_t A1 = PIN_A1;
static const uint8_t A2 = PIN_A2;
static const uint8_t A3 = PIN_A3;
static const uint8_t A4 = PIN_A4;
static const uint8_t A5 = PIN_A5;

#define ADC_RESOLUTION  14

#define PIN_AREF        (2)
#define PIN_VBAT        (32)  /* P1.00 - Battery voltage divider */

static const uint8_t AREF = PIN_AREF;

/* --- NFC (disabled, used as GPIO) --- */
#define PIN_NFC1        (33)
#define PIN_NFC2        (2)

/* --- Serial --- */
#define PIN_SERIAL1_RX  (15)  /* P0.15 */
#define PIN_SERIAL1_TX  (16)  /* P0.16 */

/* --- SPI Interfaces (WisBlock IO Slot — used by RAK14000 E-Ink) --- */
#define SPI_INTERFACES_COUNT 1

#define PIN_SPI_MISO    (29)  /* P0.29 */
#define PIN_SPI_MOSI    (30)  /* P0.30 */
#define PIN_SPI_SCK     (3)   /* P0.03 */

static const uint8_t SS   = (26);  /* P0.26 */
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t SCK  = PIN_SPI_SCK;

/* --- I2C (WisBlock Sensor Slots) --- */
#define WIRE_INTERFACES_COUNT 2

#define PIN_WIRE_SDA    (13)  /* P0.13 — primary I2C (sensor slots A-D) */
#define PIN_WIRE_SCL    (14)  /* P0.14 */

#define PIN_WIRE1_SDA   (24)  /* P0.24 — secondary I2C */
#define PIN_WIRE1_SCL   (25)  /* P0.25 */

#define WB_I2C1_SDA     PIN_WIRE_SDA
#define WB_I2C1_SCL     PIN_WIRE_SCL
#define WB_I2C2_SDA     PIN_WIRE1_SDA
#define WB_I2C2_SCL     PIN_WIRE1_SCL

/* --- WisBlock IO Slot pins (active on RAK19007 base board) --- */
#define WB_IO1          (17)  /* P0.17 — EPD DC */
#define WB_IO2          (34)  /* P1.02 — 3V3_S power enable (E-Ink, sensors) */
#define WB_IO3          (21)  /* P0.21 */
#define WB_IO4          (4)   /* P0.04 — EPD BUSY */
#define WB_IO5          (9)   /* P0.09 */
#define WB_IO6          (10)  /* P0.10 */
#define WB_A0           (5)   /* P0.05 / AIN3 */
#define WB_A1           (31)  /* P0.31 / AIN7 */
#define WB_SW1          (33)  /* P1.01 — user button on base board */

/* --- WisBlock SPI aliases (IO Slot) --- */
#define WB_SPI_CS       SS
#define WB_SPI_CLK      PIN_SPI_SCK
#define WB_SPI_MISO     PIN_SPI_MISO
#define WB_SPI_MOSI     PIN_SPI_MOSI

/* --- SX1262 LoRa Module (built into RAK4630) --- */
/* NOTE: PIN_LORA_* are NOT macros — the SX126x-Arduino library uses them
 * as struct member names. Pin values are set at runtime via lora_hardware_init().
 * Values for RAK4631:
 *   RESET = 38 (P1.06), DIO_1 = 47 (P1.15), BUSY = 46 (P1.14),
 *   NSS = 42 (P1.10), SCLK = 43 (P1.11), MISO = 45 (P1.13), MOSI = 44 (P1.12)
 */
#define RAK_LORA_RESET  38
#define RAK_LORA_DIO_1  47
#define RAK_LORA_BUSY   46
#define RAK_LORA_NSS    42
#define RAK_LORA_SCLK   43
#define RAK_LORA_MISO   45
#define RAK_LORA_MOSI   44

/* --- USB --- */
#define USB_VID         0x239A
#define USB_PID         0x8029
#define USB_MANUFACTURER "RAKwireless"
#define USB_PRODUCT     "WisCore RAK4631 Board"

#ifdef __cplusplus
}
#endif

#endif /* _VARIANT_RAK4631_ */
