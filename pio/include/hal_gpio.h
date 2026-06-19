#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * GPIO + ADC hardware abstraction. Service/gate code calls these instead of
 * Arduino pinMode/digitalWrite/analogRead so the logic is board-portable; the
 * Arduino implementation lives in hal_gpio_arduino.cpp. Pin numbers come from
 * board_pins.h. (I2C is abstracted by i2c_bus.*; SPI by hal_spi.*.)
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_GPIO_INPUT = 0,
    HAL_GPIO_INPUT_PULLUP,
    HAL_GPIO_OUTPUT,
} hal_gpio_mode_t;

void hal_gpio_mode(int pin, hal_gpio_mode_t mode);
void hal_gpio_write(int pin, bool high);
bool hal_gpio_read(int pin);

/* Averaged ADC read at 12-bit resolution with the internal 3.0 V reference
 * (restores the default reference afterwards). Returns raw counts [0..4095]. */
uint16_t hal_adc_read_raw_3v0(int pin, int samples);

#ifdef __cplusplus
}
#endif
