/*
 * LoRaWAN service — stub implementation for gate bring-up.
 * This is functionally identical to the ESP-IDF stub version.
 *
 * TODO: Replace with real SX126x-Arduino OTAA implementation:
 *   #include <LoRaWan-RAK4630.h>
 *   Use lmh_init(), lmh_setDevEui(), lmh_setAppKey(), lmh_join(), lmh_send()
 */

#include <Arduino.h>
#include <string.h>

#include "lorawan_service.h"

static const char *TAG = "LORAWAN";

static lorawan_state_t s_state = LORAWAN_STATE_IDLE;
static uint64_t s_join_started_ms;
static uint64_t s_next_retry_ms;
static lorawan_metrics_t s_metrics;

esp_err_t lorawan_service_init(void)
{
    s_state = LORAWAN_STATE_IDLE;
    s_join_started_ms = 0;
    s_next_retry_ms = 0;
    memset(&s_metrics, 0, sizeof(s_metrics));
    s_metrics.backend_active = APP_LORAWAN_BACKEND_ACTIVE;

    ESP_LOGI(TAG, "init_stub region=AS923-1 backend_active=%d", s_metrics.backend_active);
    return ESP_OK;
}

esp_err_t lorawan_service_set_backend_active(bool active)
{
    s_metrics.backend_active = active;
    ESP_LOGI(TAG, "backend_active_set value=%d", active);

    if (!active && s_state == LORAWAN_STATE_JOINED) {
        s_state = LORAWAN_STATE_IDLE;
        s_next_retry_ms = 0;
    }
    return ESP_OK;
}

void lorawan_service_get_metrics(lorawan_metrics_t *out)
{
    if (out != NULL) {
        *out = s_metrics;
    }
}

void lorawan_service_process(uint64_t now_ms)
{
    if (s_state == LORAWAN_STATE_JOINED) return;
    if (now_ms < s_next_retry_ms) return;

    if (s_state == LORAWAN_STATE_IDLE) {
        s_metrics.join_attempts++;
        if (!s_metrics.backend_active) {
            s_metrics.join_failures++;
            s_metrics.retries++;
            s_next_retry_ms = now_ms + (uint64_t)APP_LORAWAN_JOIN_BACKOFF_MS;
            ESP_LOGW(TAG, "join_attempt=%lu backend_active=0 retry_in_ms=%d",
                     (unsigned long)s_metrics.join_attempts, APP_LORAWAN_JOIN_BACKOFF_MS);
            return;
        }
        s_state = LORAWAN_STATE_JOINING;
        s_join_started_ms = now_ms;
        ESP_LOGI(TAG, "join_start attempt=%lu", (unsigned long)s_metrics.join_attempts);
        return;
    }

    if (s_state == LORAWAN_STATE_JOINING) {
        if (!s_metrics.backend_active) {
            s_state = LORAWAN_STATE_IDLE;
            s_metrics.join_failures++;
            s_metrics.retries++;
            s_next_retry_ms = now_ms + (uint64_t)APP_LORAWAN_JOIN_BACKOFF_MS;
            ESP_LOGW(TAG, "join_aborted backend_inactive retry_in_ms=%d", APP_LORAWAN_JOIN_BACKOFF_MS);
            return;
        }

        if (now_ms - s_join_started_ms >= (uint64_t)APP_LORAWAN_JOIN_SUCCESS_DELAY_MS) {
            s_state = LORAWAN_STATE_JOINED;
            s_metrics.join_successes++;
            s_metrics.last_join_ms = now_ms;
            ESP_LOGI(TAG, "join_success attempts=%lu", (unsigned long)s_metrics.join_attempts);
        }
    }
}

lorawan_state_t lorawan_service_state(void)
{
    return s_state;
}

bool lorawan_service_is_joined(void)
{
    return s_state == LORAWAN_STATE_JOINED;
}

esp_err_t lorawan_service_send(const uint8_t *payload, size_t len)
{
    if (payload == NULL || len == 0) return ESP_ERR_INVALID_ARG;
    if (!lorawan_service_is_joined()) {
        s_metrics.uplink_fail++;
        return ESP_ERR_INVALID_STATE;
    }
    if (!s_metrics.backend_active) {
        s_metrics.uplink_fail++;
        return ESP_ERR_INVALID_STATE;
    }

    s_metrics.uplink_ok++;
    ESP_LOGI(TAG, "uplink_stub_ok bytes=%u", (unsigned)len);
    return ESP_OK;
}
