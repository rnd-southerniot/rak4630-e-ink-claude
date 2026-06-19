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

uint16_t hal_adc_read_raw_3v0(int pin, int samples)
{
    if (samples < 1) samples = 1;
    analogReference(AR_INTERNAL_3_0);
    analogReadResolution(12);

    uint32_t acc = 0;
    for (int i = 0; i < samples; i++) {
        acc += (uint32_t)analogRead(pin);
    }
    analogReference(AR_DEFAULT);
    return (uint16_t)(acc / (uint32_t)samples);
}
