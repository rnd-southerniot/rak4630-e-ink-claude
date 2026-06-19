/*
 * Heartbeat LED — Arduino implementation for nRF52840 (RAK4630)
 * Uses cooperative scheduling (polled from loop) instead of FreeRTOS task.
 * The nRF52 Arduino BSP does have FreeRTOS, but keeping it simple.
 */

#include <Arduino.h>  /* millis() */

#include "heartbeat.h"
#include "board_pins.h"
#include "hal_gpio.h"

static const char *TAG = "APP";
static volatile uint32_t s_toggle_count = 0;
static volatile bool s_running = false;
static bool s_level = false;
static uint32_t s_last_toggle_ms = 0;

esp_err_t heartbeat_start(void)
{
    if (PIN_HEARTBEAT_LED < 0) {
        ESP_LOGW(TAG, "heartbeat_disabled gpio=-1");
        return ESP_OK;
    }

    hal_gpio_mode(PIN_HEARTBEAT_LED, HAL_GPIO_OUTPUT);
    hal_gpio_write(PIN_HEARTBEAT_LED, false);
    s_toggle_count = 0;
    s_running = true;
    s_last_toggle_ms = millis();

    ESP_LOGI(TAG, "heartbeat_started gpio=%d", PIN_HEARTBEAT_LED);
    return ESP_OK;
}

void heartbeat_update(void)
{
    if (!s_running) return;

    uint32_t now = millis();
    if (now - s_last_toggle_ms >= 500) {
        s_level = !s_level;
        hal_gpio_write(PIN_HEARTBEAT_LED, s_level);
        s_toggle_count++;
        s_last_toggle_ms = now;
    }
}

uint32_t heartbeat_get_toggle_count(void)
{
    return s_toggle_count;
}

bool heartbeat_is_running(void)
{
    return s_running;
}
