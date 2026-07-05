#include <stdint.h>

#include "app_gate.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#if CONFIG_APP_LORA_SMOKE
#include "lora.h"
#endif

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

#if CONFIG_APP_LORA_SMOKE
/* P2 SX1262 LoRaWAN smoke: init -> OTAA join (indefinite backoff) -> periodic uplink. Bypasses the
 * gate runner; proves the ported RadioLib+EspHalS3 radio stack. Does not return. */
static void lora_smoke_run(void)
{
    ESP_LOGW(TAG, "LORA SMOKE (P2): init -> join -> uplink loop");
    if (!lora_is_provisioned()) {
        ESP_LOGW(TAG, "no OTAA creds (placeholder) — fill firmware/main/lora_credentials.h to join");
    }
    ESP_ERROR_CHECK(lora_init());
    uint32_t backoff_s = 10;
    while (lora_join() != ESP_OK) {
        ESP_LOGW(TAG, "join retry in %us", (unsigned)backoff_s);
        vTaskDelay(pdMS_TO_TICKS(backoff_s * 1000));
        if (backoff_s < 60) {
            backoff_s = (backoff_s == 10) ? 30 : 60;
        }
    }
    for (uint32_t i = 0;; ++i) {
        const uint8_t pl[4] = {0xA5, (uint8_t)(i >> 8), (uint8_t)i, 0x5A};
        lora_send(pl, sizeof(pl));
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
#endif

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    log_deveui_from_mac();

#if CONFIG_APP_LORA_SMOKE
    lora_smoke_run(); /* does not return */
#endif

    app_gate_ctx_t gate_ctx;
    ESP_ERROR_CHECK(app_gate_init(&gate_ctx));

    ESP_LOGI(TAG, "gate_runner_started gate=%d", (int)gate_ctx.selected);

    while (true) {
        app_gate_tick(&gate_ctx, now_ms());
        vTaskDelay(pdMS_TO_TICKS(CONFIG_APP_GATE_TICK_MS));
    }
}
