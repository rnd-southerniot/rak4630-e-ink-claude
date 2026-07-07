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
#include "buttons.h"
#include "device_profile.h"
#include "lora.h"
#include "profile_reader.h"
#include "profile_store.h"
#include "prov_console.h"
#include "telemetry.h"
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
/* Sleep up to `ms`, but return early on a RAK14000 button press (S1/S2/S3) so the field loop sends an
 * immediate uplink. Returns the button (1..3) that woke it, or 0 on timeout. */
static uint8_t field_wait_or_button(uint32_t ms)
{
    const uint32_t step = 100;
    for (uint32_t t = 0; t < ms; t += step) {
        uint8_t btn = 0;
        if (buttons_take(&btn)) {
            ESP_LOGI(TAG, "button S%u pressed -> immediate uplink", (unsigned)btn);
            return btn;
        }
        vTaskDelay(pdMS_TO_TICKS(step));
    }
    return 0;
}

/* P2/P3 LoRa field path: start the esp> provisioning console, wait if unprovisioned, else init the
 * SX1262 -> OTAA join (indefinite backoff) -> periodic uplink. Bypasses the gate runner. Credentials
 * come from NVS 'prov' (console / CRM) with the compiled lora_credentials.h as fallback. No return. */
static void lora_field_run(void)
{
    ESP_ERROR_CHECK(prov_console_init()); /* esp> console runs on its own task — provision anytime */
    ESP_ERROR_CHECK(buttons_init());      /* RAK14000 S1/S2/S3 — a press forces an immediate uplink */

    if (!lora_is_provisioned()) {
        ESP_LOGW(TAG, "AWAITING PROVISIONING — NVS 'prov' empty and the compiled key is a placeholder.");
        ESP_LOGW(TAG, "Provision over esp> :  prov creds <devEui> <joinEui> <appKey>  then  prov-done");
        for (;;) {
            ESP_LOGW(TAG, "AWAITING PROVISIONING (esp> prov creds ... ; prov-done)");
            vTaskDelay(pdMS_TO_TICKS(30000));
        }
    }

    /* A provisioned device profile (ADR-006) drives a generic sensor read + ADR-005 encode; absent
     * one, uplink a heartbeat test payload (prov-profile <hex> to enable real telemetry). */
    static dp_profile_storage_t s_prof;
    const bool have_profile = profile_store_load(&s_prof);
    const device_profile_t *prof = have_profile ? &s_prof.profile : NULL;
    if (have_profile) {
        ESP_LOGW(TAG,
                 "field profile: device_byte=0x%02X bus=%u sensor_type=%u %u meas -> %u B payload",
                 prof->device_byte, prof->bus_kind, prof->sensor_type, prof->n_meas, prof->total_len);
        ESP_ERROR_CHECK(profile_reader_init());
    } else {
        ESP_LOGW(TAG, "no device profile in NVS — heartbeat test payload (prov-profile to enable)");
    }

    ESP_LOGW(TAG, "LoRa field: creds present -> init -> join -> uplink");
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
        if (have_profile) {
            float values[DP_MAX_MEAS];
            uint8_t flags = 0;
            if (profile_read(prof, values, DP_MAX_MEAS) != ESP_OK) {
                flags |= TELEMETRY_FLAG_STALE;
                ESP_LOGW(TAG, "sensor read failed -> STALE");
            }
            uint8_t buf[64];
            const size_t len = dp_encode_payload(prof, values, flags, buf, sizeof(buf));
            ESP_LOGI(TAG, "[%lu] profile dev=0x%02X %u meas v[0]=%.2f v[1]=%.2f -> %u B",
                     (unsigned long)i, prof->device_byte, prof->n_meas,
                     prof->n_meas > 0 ? (double)values[0] : 0.0, prof->n_meas > 1 ? (double)values[1] : 0.0,
                     (unsigned)len);
            if (len > 0) {
                lora_send(buf, len);
            }
        } else {
            const uint8_t pl[4] = {0xA5, (uint8_t)(i >> 8), (uint8_t)i, 0x5A};
            lora_send(pl, sizeof(pl));
        }
        /* Sample interval, but a button press sends immediately (next loop iteration). */
        field_wait_or_button(30000);
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
    lora_field_run(); /* does not return */
#endif

    app_gate_ctx_t gate_ctx;
    ESP_ERROR_CHECK(app_gate_init(&gate_ctx));

    ESP_LOGI(TAG, "gate_runner_started gate=%d", (int)gate_ctx.selected);

    while (true) {
        app_gate_tick(&gate_ctx, now_ms());
        vTaskDelay(pdMS_TO_TICKS(CONFIG_APP_GATE_TICK_MS));
    }
}
