#pragma once

#include <stdint.h>

/*
 * SPI master HAL (MSB-first, mode 0) used by the display driver, so the driver
 * doesn't call the Arduino SPI object directly. Arduino implementation in
 * hal_spi_arduino.cpp.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Begin the SPI bus. On nRF52 the pins come from the board variant (args ignored);
 * on ESP32 the bus is remappable, so the caller's sclk/miso/mosi pins are applied
 * (cs is managed separately via hal_gpio). */
void hal_spi_begin(int sclk, int miso, int mosi);
void hal_spi_begin_transaction(uint32_t hz);   /* MSB-first, SPI mode 0 */
uint8_t hal_spi_transfer(uint8_t value);
void hal_spi_end_transaction(void);

#ifdef __cplusplus
}
#endif
