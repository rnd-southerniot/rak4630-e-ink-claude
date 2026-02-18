#include <stdint.h>

#include "app_gate.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

static const char *TAG = "APP";

static uint64_t now_ms(void)
{
    return (uint64_t)(esp_timer_get_time() / 1000ULL);
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    app_gate_ctx_t gate_ctx;
    ESP_ERROR_CHECK(app_gate_init(&gate_ctx));

    ESP_LOGI(TAG, "gate_runner_started gate=%d", (int)gate_ctx.selected);

    while (true) {
        app_gate_tick(&gate_ctx, now_ms());
        vTaskDelay(pdMS_TO_TICKS(CONFIG_APP_GATE_TICK_MS));
    }
}
