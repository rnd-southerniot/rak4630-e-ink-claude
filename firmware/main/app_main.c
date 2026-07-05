#include <stdint.h>

#include "app_gate.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

static const char *TAG = "APP";

static uint64_t now_ms(void)
{
    return (uint64_t)(esp_timer_get_time() / 1000ULL);
}

/* Log the LoRaWAN DevEUI derived from the ESP32-S3 factory MAC (EUI-48 -> EUI-64 with FF:FE inserted)
 * early, so the CRM factory step can read it back before any join. The SX1262 has no DevEUI. */
static void log_deveui_from_mac(void)
{
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);
    const uint8_t eui[8] = {mac[0], mac[1], mac[2], 0xFF, 0xFE, mac[3], mac[4], mac[5]};
    ESP_LOGW(TAG, "DevEUI (from MAC): %02x%02x%02x%02x%02x%02x%02x%02x", eui[0], eui[1], eui[2],
             eui[3], eui[4], eui[5], eui[6], eui[7]);
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    log_deveui_from_mac();

    app_gate_ctx_t gate_ctx;
    ESP_ERROR_CHECK(app_gate_init(&gate_ctx));

    ESP_LOGI(TAG, "gate_runner_started gate=%d", (int)gate_ctx.selected);

    while (true) {
        app_gate_tick(&gate_ctx, now_ms());
        vTaskDelay(pdMS_TO_TICKS(CONFIG_APP_GATE_TICK_MS));
    }
}
