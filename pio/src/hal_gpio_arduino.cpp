/*
 * GPIO + ADC HAL — Arduino (nRF52840) implementation.
 */
#include <Arduino.h>

#include "hal_gpio.h"

void hal_gpio_mode(int pin, hal_gpio_mode_t mode)
{
    switch (mode) {
    case HAL_GPIO_INPUT:        pinMode(pin, INPUT); break;
    case HAL_GPIO_INPUT_PULLUP: pinMode(pin, INPUT_PULLUP); break;
    case HAL_GPIO_OUTPUT:       pinMode(pin, OUTPUT); break;
    }
}

void hal_gpio_write(int pin, bool high)
{
    digitalWrite(pin, high ? HIGH : LOW);
}

bool hal_gpio_read(int pin)
{
    return digitalRead(pin) == HIGH;
}
