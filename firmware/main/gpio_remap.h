/*
 * gpio_remap.h — SX1262 LoRa pin map for senseflow-eink-node (RAK3312, ESP32-S3).
 *
 * The RAK3312 WisDuo module wires the on-package SX1262 identically to the RAK3112
 * (careflow-modbus-node) — bench-confirmed pins. Raw ESP32-S3 GPIO numbers.
 * (Consumed by lora.cpp via EspHalS3 + the SX1262 Module constructor.)
 */
#ifndef GPIO_REMAP_H
#define GPIO_REMAP_H

#define PIN_LORA_MISO  3  /* SX1262 MISO */
#define PIN_LORA_SCK   5  /* SX1262 SCK  */
#define PIN_LORA_MOSI  6  /* SX1262 MOSI */
#define PIN_LORA_NSS   7  /* SX1262 NSS / CS */
#define PIN_LORA_RESET 8  /* SX1262 NRESET (active-low) */
#define PIN_LORA_DIO1  47 /* SX1262 DIO1 (IRQ) */
#define PIN_LORA_BUSY  48 /* SX1262 BUSY */

/* RAK3312 exposes an explicit ANT_SW GPIO. careflow drives the RF switch via SX1262 DIO2
 * (setDio2AsRfSwitch) which is the proven WisDuo config; keep this as the fallback if the
 * P2 join shows no RX (switch DIO2→explicit ANT_SW). */
#define PIN_LORA_ANT_SW 4

#endif /* GPIO_REMAP_H */
