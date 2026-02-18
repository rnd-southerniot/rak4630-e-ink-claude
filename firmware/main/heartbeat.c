#include "heartbeat.h"

#include <stdatomic.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "APP";

static _Atomic uint32_t s_toggle_count;
static _Atomic bool s_running;

static void heartbeat_task(void *arg)
{
    (void)arg;
    const int pin = CONFIG_APP_HEARTBEAT_GPIO;
    bool level = false;

    while (true) {
        level = !level;
        gpio_set_level(pin, level);
        s_toggle_count++;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

esp_err_t heartbeat_start(void)
{
    if (CONFIG_APP_HEARTBEAT_GPIO < 0) {
        ESP_LOGW(TAG, "heartbeat_disabled gpio=-1");
        return ESP_OK;
    }

    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << CONFIG_APP_HEARTBEAT_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));

    s_toggle_count = 0;
    BaseType_t ok = xTaskCreate(heartbeat_task, "heartbeat_task", 2048, NULL, 4, NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "heartbeat_task_create_failed");
        s_running = false;
        return ESP_FAIL;
    }

    s_running = true;
    ESP_LOGI(TAG, "heartbeat_started gpio=%d", CONFIG_APP_HEARTBEAT_GPIO);
    return ESP_OK;
}

uint32_t heartbeat_get_toggle_count(void)
{
    return s_toggle_count;
}

bool heartbeat_is_running(void)
{
    return s_running;
}
