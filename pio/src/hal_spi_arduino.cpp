/*
 * SPI HAL — Arduino (nRF52840) implementation over the default SPI object.
 */
#include <Arduino.h>
#include <SPI.h>

#include "hal_spi.h"

void hal_spi_begin(void)
{
    SPI.begin();
}

void hal_spi_begin_transaction(uint32_t hz)
{
    SPI.beginTransaction(SPISettings(hz, MSBFIRST, SPI_MODE0));
}

uint8_t hal_spi_transfer(uint8_t value)
{
    return SPI.transfer(value);
}

void hal_spi_end_transaction(void)
{
    SPI.endTransaction();
}
