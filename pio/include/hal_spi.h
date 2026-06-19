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

void hal_spi_begin(void);
void hal_spi_begin_transaction(uint32_t hz);   /* MSB-first, SPI mode 0 */
uint8_t hal_spi_transfer(uint8_t value);
void hal_spi_end_transaction(void);

#ifdef __cplusplus
}
#endif
