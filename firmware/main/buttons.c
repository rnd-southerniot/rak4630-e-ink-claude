/* buttons.c — RAK14000 S1/S2/S3 debounced FALLING-edge inputs (see buttons.h). */
#include "buttons.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "gpio_remap.h"

static const char *TAG = "btn";

#define DEBOUNCE_US 40000 /* 40 ms — drop contact bounce (ISR-level, per button) */

static const int s_pins[3] = {PIN_BTN_S1, PIN_BTN_S2, PIN_BTN_S3};
static volatile int64_t s_last_us[3];
static QueueHandle_t s_queue;

static void IRAM_ATTR button_isr(void *arg)
{
    const uint32_t idx = (uint32_t)(uintptr_t)arg; /* 0..2 */
    const int64_t now = esp_timer_get_time();
    if (now - s_last_us[idx] < DEBOUNCE_US) {
        return; /* debounce */
    }
    s_last_us[idx] = now;
    const uint8_t btn = (uint8_t)(idx + 1); /* 1..3 = S1..S3 */
    BaseType_t hp = pdFALSE;
    xQueueSendFromISR(s_queue, &btn, &hp);
    if (hp) {
        portYIELD_FROM_ISR();
    }
}

esp_err_t buttons_init(void)
{
    s_queue = xQueueCreate(8, sizeof(uint8_t));
    if (s_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }
    const gpio_config_t io = {
        .pin_bit_mask = (1ULL << PIN_BTN_S1) | (1ULL << PIN_BTN_S2) | (1ULL << PIN_BTN_S3),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, /* module has external pull-ups; this is belt-and-braces */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE, /* active-low: press = falling edge */
    };
    ESP_ERROR_CHECK(gpio_config(&io));

    /* The ISR service may already be installed (e.g. by the RadioLib DIO1 HAL in a LoRa build). */
    const esp_err_t svc = gpio_install_isr_service(0);
    if (svc != ESP_OK && svc != ESP_ERR_INVALID_STATE) {
        return svc;
    }
    for (uint32_t i = 0; i < 3; ++i) {
        ESP_ERROR_CHECK(gpio_isr_handler_add(s_pins[i], button_isr, (void *)(uintptr_t)i));
    }
    ESP_LOGI(TAG, "buttons ready: S1=%d S2=%d S3=%d (active-low, FALLING, 40ms debounce)",
             PIN_BTN_S1, PIN_BTN_S2, PIN_BTN_S3);
    return ESP_OK;
}

bool buttons_take(uint8_t *btn)
{
    if (s_queue == NULL || btn == NULL) {
        return false;
    }
    return xQueueReceive(s_queue, btn, 0) == pdTRUE;
}
