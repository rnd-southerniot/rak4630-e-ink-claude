/*
 * LoRaWAN service — real SX126x-Arduino OTAA implementation for RAK4630 (SX1262).
 *
 * Replaces the former bring-up stub. The public API in lorawan_service.h is
 * unchanged, so gates 6/7/9 in app_gate.cpp call this backend without edits.
 *
 * Behavior:
 *   - OTAA join via lmh_join() against ChirpStack v4, region AS923-1.
 *   - Uplinks are CONFIRMED. uplink_ok increments only on the downlink ACK
 *     (lmh_conf_result callback), never on transmit — this lets gates 6 and 9
 *     prove reception, not just transmission.
 *   - lorawan_service_set_backend_active() gates whether a join is issued,
 *     preserving gate 7's buffer-before-join / flush-after-join sequence.
 *
 * Credentials (DEVEUI / APPEUI(JoinEUI) / APPKEY) are injected as build flags
 * by pio/scripts/inject_credentials.py from firmware/.env (gitignored). They
 * arrive as uppercase hex strings and are parsed MSB-first to match the values
 * shown in the ChirpStack device console.
 */

#include <Arduino.h>
#include <string.h>

#include <LoRaWan-Arduino.h>

#include "lorawan_service.h"
#include "board.h"

/* The SX1262 radio bring-up (pins, SPI, TCXO, antenna switch) is board-specific
 * and lives in the board package (src/board/<board>/). lorawan_service is now
 * board-agnostic: it calls board_radio_init() then drives the shared LoRaMac. */

static const char *TAG = "LORAWAN";

/* ---- Credentials (build-flag injected; safe placeholders for offline builds) ---- */
#ifndef APP_DEVEUI
#define APP_DEVEUI "0000000000000000"
#endif
#ifndef APP_JOINEUI
#define APP_JOINEUI "0000000000000000"
#endif
#ifndef APP_APPKEY
#define APP_APPKEY "00000000000000000000000000000000"
#endif

/* ---- Tunables (overridable via -D flags) ---- */
#ifndef APP_LORAWAN_SUBBAND
#define APP_LORAWAN_SUBBAND 1
#endif
#ifndef APP_LORAWAN_CONF_RETRIES
#define APP_LORAWAN_CONF_RETRIES 8
#endif
#ifndef APP_LORAWAN_JOIN_TRIALS
#define APP_LORAWAN_JOIN_TRIALS 8
#endif
#ifndef APP_LORAWAN_JOIN_BACKOFF_MS
#define APP_LORAWAN_JOIN_BACKOFF_MS 15000
#endif
#ifndef APP_LORAWAN_APP_PORT
#define APP_LORAWAN_APP_PORT 2
#endif

#define LORAWAN_TX_BUFFER_SIZE 64

/* ---- State ---- */
static lorawan_state_t s_state = LORAWAN_STATE_IDLE;
static lorawan_metrics_t s_metrics;
static bool s_initialized = false;
static bool s_join_in_flight = false;
static uint64_t s_next_join_ms = 0;

static uint8_t s_dev_eui[8];
static uint8_t s_join_eui[8];
static uint8_t s_app_key[16];

static uint8_t s_tx_buffer[LORAWAN_TX_BUFFER_SIZE];
static lmh_app_data_t s_tx_data = {s_tx_buffer, 0, APP_LORAWAN_APP_PORT, 0, 0};

/* ---- Hex parsing (MSB-first, as displayed by ChirpStack) ---- */
static int hex_nibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool parse_hex(const char *hex, uint8_t *out, size_t out_len)
{
    if (hex == NULL) return false;
    if (strlen(hex) != out_len * 2) return false;
    for (size_t i = 0; i < out_len; i++) {
        int hi = hex_nibble(hex[i * 2]);
        int lo = hex_nibble(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0) return false;
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return true;
}

/* ---- LoRaMac helper callbacks (invoked asynchronously by the library) ---- */
static void lw_has_joined(void)
{
    s_state = LORAWAN_STATE_JOINED;
    s_join_in_flight = false;
    s_metrics.join_successes++;
    s_metrics.last_join_ms = (uint64_t)millis();
    lmh_class_request(CLASS_A);
    ESP_LOGI(TAG, "join_success attempts=%lu successes=%lu",
             (unsigned long)s_metrics.join_attempts, (unsigned long)s_metrics.join_successes);
}

static void lw_join_failed(void)
{
    s_state = LORAWAN_STATE_IDLE;
    s_join_in_flight = false;
    s_metrics.join_failures++;
    s_next_join_ms = (uint64_t)millis() + (uint64_t)APP_LORAWAN_JOIN_BACKOFF_MS;
    ESP_LOGW(TAG, "join_failed attempts=%lu failures=%lu retry_in_ms=%d",
             (unsigned long)s_metrics.join_attempts, (unsigned long)s_metrics.join_failures,
             APP_LORAWAN_JOIN_BACKOFF_MS);
}

static void lw_rx_data(lmh_app_data_t *app_data)
{
    ESP_LOGI(TAG, "rx port=%d size=%d rssi=%d snr=%d",
             app_data->port, app_data->buffsize, app_data->rssi, app_data->snr);
}

static void lw_confirm_class(DeviceClass_t Class)
{
    ESP_LOGI(TAG, "class_confirmed class=%c", "ABC"[Class]);
}

static void lw_conf_result(bool result)
{
    if (result) {
        s_metrics.uplink_ok++;
        ESP_LOGI(TAG, "uplink_ack_ok uplink_ok=%lu", (unsigned long)s_metrics.uplink_ok);
    } else {
        s_metrics.uplink_fail++;
        ESP_LOGW(TAG, "uplink_ack_fail uplink_fail=%lu", (unsigned long)s_metrics.uplink_fail);
    }
}

static lmh_callback_t s_lmh_callbacks = {
    BoardGetBatteryLevel,
    BoardGetUniqueId,
    BoardGetRandomSeed,
    lw_rx_data,
    lw_has_joined,
    lw_confirm_class,
    lw_join_failed,
    NULL,           /* lmh_unconf_finished — unused (we send confirmed) */
    lw_conf_result,
};

static lmh_param_t s_lmh_param = {
    LORAWAN_ADR_ON,
    LORAWAN_DEFAULT_DATARATE,
    LORAWAN_PUBLIC_NETWORK,
    APP_LORAWAN_JOIN_TRIALS,
    LORAWAN_DEFAULT_TX_POWER,
    LORAWAN_DUTYCYCLE_OFF,
};

/* ---- Public API ---- */
esp_err_t lorawan_service_init(void)
{
    s_state = LORAWAN_STATE_IDLE;
    s_join_in_flight = false;
    s_next_join_ms = 0;
    memset(&s_metrics, 0, sizeof(s_metrics));
    s_metrics.backend_active = true;

    if (!parse_hex(APP_DEVEUI, s_dev_eui, sizeof(s_dev_eui)) ||
        !parse_hex(APP_JOINEUI, s_join_eui, sizeof(s_join_eui)) ||
        !parse_hex(APP_APPKEY, s_app_key, sizeof(s_app_key))) {
        ESP_LOGE(TAG, "credential_parse_failed check firmware/.env (DEVEUI/JOINEUI/APPKEY hex length)");
        return ESP_ERR_INVALID_ARG;
    }

    if (board_radio_init() != ESP_OK) {
        return ESP_FAIL;
    }

    lmh_setDevEui(s_dev_eui);
    lmh_setAppEui(s_join_eui);
    lmh_setAppKey(s_app_key);

    uint32_t err = lmh_init(&s_lmh_callbacks, s_lmh_param, true, CLASS_A, LORAMAC_REGION_AS923);
    if (err != 0) {
        ESP_LOGE(TAG, "lmh_init_failed code=%lu", (unsigned long)err);
        return ESP_FAIL;
    }

    if (!lmh_setSubBandChannels(APP_LORAWAN_SUBBAND)) {
        ESP_LOGW(TAG, "set_subband_failed subband=%d", APP_LORAWAN_SUBBAND);
    }
    lmh_setConfRetries(APP_LORAWAN_CONF_RETRIES);

    s_initialized = true;
    ESP_LOGI(TAG, "init_ok region=AS923-1 subband=%d conf_retries=%d backend_active=%d",
             APP_LORAWAN_SUBBAND, APP_LORAWAN_CONF_RETRIES, s_metrics.backend_active);
    return ESP_OK;
}

esp_err_t lorawan_service_set_backend_active(bool active)
{
    s_metrics.backend_active = active;
    ESP_LOGI(TAG, "backend_active_set value=%d", active);

    if (!active && s_state != LORAWAN_STATE_IDLE) {
        /* Stop issuing joins / uplinks; a later re-activation re-issues lmh_join(). */
        s_state = LORAWAN_STATE_IDLE;
        s_join_in_flight = false;
        s_next_join_ms = 0;
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
    if (!s_initialized) return;
    if (!s_metrics.backend_active) return;
    if (s_state == LORAWAN_STATE_JOINED) return;
    if (s_join_in_flight) return;
    if (now_ms < s_next_join_ms) return;

    /* (Re)issue the OTAA join request. The library retries nb_trials times
     * internally, then signals lw_has_joined() or lw_join_failed(). */
    if (s_metrics.join_attempts > 0) {
        s_metrics.retries++;
    }
    s_metrics.join_attempts++;
    s_state = LORAWAN_STATE_JOINING;
    s_join_in_flight = true;
    ESP_LOGI(TAG, "join_start attempt=%lu", (unsigned long)s_metrics.join_attempts);
    lmh_join();
}

lorawan_state_t lorawan_service_state(void)
{
    return s_state;
}

bool lorawan_service_is_joined(void)
{
    return (s_state == LORAWAN_STATE_JOINED) && (lmh_join_status_get() == LMH_SET);
}

esp_err_t lorawan_service_send(const uint8_t *payload, size_t len)
{
    if (payload == NULL || len == 0) return ESP_ERR_INVALID_ARG;
    if (len > sizeof(s_tx_buffer)) return ESP_ERR_INVALID_ARG;
    if (!s_metrics.backend_active) {
        s_metrics.uplink_fail++;
        return ESP_ERR_INVALID_STATE;
    }
    if (!lorawan_service_is_joined()) {
        s_metrics.uplink_fail++;
        return ESP_ERR_INVALID_STATE;
    }

    memcpy(s_tx_buffer, payload, len);
    s_tx_data.buffer = s_tx_buffer;
    s_tx_data.buffsize = (uint8_t)len;
    s_tx_data.port = APP_LORAWAN_APP_PORT;

    lmh_error_status st = lmh_send(&s_tx_data, LMH_CONFIRMED_MSG);
    if (st != LMH_SUCCESS) {
        s_metrics.uplink_fail++;
        ESP_LOGW(TAG, "uplink_enqueue_fail status=%d bytes=%u", (int)st, (unsigned)len);
        return ESP_FAIL;
    }

    /* Queued for confirmed TX. uplink_ok is incremented later by lw_conf_result()
     * when (and only when) the network ACK arrives. */
    ESP_LOGI(TAG, "uplink_sent_confirmed bytes=%u awaiting_ack=1", (unsigned)len);
    return ESP_OK;
}
