#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_types.h"
#include "app_payload.h"
#include "esp_err.h"

typedef enum {
    APP_GATE_0_ENV = 0,
    APP_GATE_1_HEARTBEAT = 1,
    APP_GATE_2_DISPLAY_SMOKE = 2,
    APP_GATE_2P1_I2C_SMOKE = 21,
    APP_GATE_3_I2C_PRESENCE = 3,
    APP_GATE_4_SENSOR_PIPELINE = 4,
    APP_GATE_5_PAYLOAD_V1 = 5,
    APP_GATE_6_LORAWAN_JOIN_UPLINK = 6,
    APP_GATE_7_RELIABILITY_BUFFER = 7,
    APP_GATE_8_FUOTA_SCAFFOLD = 8,
    APP_GATE_9_LIVE_PUBLISH = 9,
} app_gate_t;

typedef struct {
    app_gate_t selected;
    bool pass;
    bool halted;
    uint32_t failures;

    uint64_t start_ms;
    uint64_t last_progress_log_ms;
    uint64_t last_idle_log_ms;
    uint64_t last_sample_ms;
    uint64_t last_uplink_ms;
    uint64_t last_refresh_ms;

    bool i2c_scan_done;
    bool identity_ok;
    bool gate5_uplink_done;
    bool gate6_backend_enabled;
    bool gate6_buffer_valid;

    uint32_t heartbeat_last_toggle;
    uint32_t missing_sgp40_count;
    uint32_t missing_bmp280_count;
    uint32_t sensor_ok;
    uint32_t sensor_fail;
    uint32_t payload_ok;
    uint32_t payload_fail;
    uint32_t display_updates;
    uint32_t buffered_count;
    uint32_t flushed_count;

    sensor_sample_t latest_sample;
    sensor_sample_t last_rendered;
    bool has_latest_sample;
    bool has_last_rendered;
    uint8_t latest_payload[APP_PAYLOAD_V1_LEN];
    size_t latest_payload_len;
} app_gate_ctx_t;

esp_err_t app_gate_init(app_gate_ctx_t *ctx);
void app_gate_tick(app_gate_ctx_t *ctx, uint64_t now_ms);
const char *app_gate_name(app_gate_t gate);
bool app_gate_is_halted(const app_gate_ctx_t *ctx);
