#include "app_gate.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "app_payload.h"
#include "display_service.h"
#include "esp_check.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_system.h"
#include "gate_id_map.h"
#include "heartbeat.h"
#include "i2c_bus.h"
#include "lorawan_service.h"
#include "sensor_service.h"

static const char *TAG = "APP";

#define APP_GATE_PROGRESS_LOG_MS 2000ULL

static bool gate3_expect_sgp40(void)
{
    return (CONFIG_APP_GATE3_EXPECTED_DEVICES == 1 || CONFIG_APP_GATE3_EXPECTED_DEVICES == 3);
}

static bool gate2p1_expect_sgp40(void)
{
    return (CONFIG_APP_GATE2P1_EXPECTED_DEVICES == 1 || CONFIG_APP_GATE2P1_EXPECTED_DEVICES == 3);
}

static bool gate2p1_expect_bmp280(void)
{
    return (CONFIG_APP_GATE2P1_EXPECTED_DEVICES == 2 || CONFIG_APP_GATE2P1_EXPECTED_DEVICES == 3);
}

static bool gate3_expect_bmp280(void)
{
    return (CONFIG_APP_GATE3_EXPECTED_DEVICES == 2 || CONFIG_APP_GATE3_EXPECTED_DEVICES == 3);
}

static bool gate4_expect_sgp40(void)
{
    return (CONFIG_APP_GATE4_EXPECTED_DEVICES == 1 || CONFIG_APP_GATE4_EXPECTED_DEVICES == 3);
}

static bool gate4_expect_bmp280(void)
{
    return (CONFIG_APP_GATE4_EXPECTED_DEVICES == 2 || CONFIG_APP_GATE4_EXPECTED_DEVICES == 3);
}

static bool gate9_expect_sgp40(void)
{
    return (CONFIG_APP_GATE9_EXPECTED_DEVICES == 1 || CONFIG_APP_GATE9_EXPECTED_DEVICES == 3);
}

static bool gate9_expect_bmp280(void)
{
    return (CONFIG_APP_GATE9_EXPECTED_DEVICES == 2 || CONFIG_APP_GATE9_EXPECTED_DEVICES == 3);
}

static bool gate9_sample_in_range(const sensor_sample_t *sample)
{
    if (sample == NULL) {
        return false;
    }

    return (
        sample->voc_index >= 0.0f && sample->voc_index <= 500.0f &&
        sample->pressure_pa >= 80000.0f && sample->pressure_pa <= 120000.0f &&
        sample->temperature_c >= -40.0f && sample->temperature_c <= 85.0f &&
        sample->battery_v >= 3.0f && sample->battery_v <= 5.0f);
}

static void gate9_seed_sample_defaults(app_gate_ctx_t *ctx, sensor_sample_t *sample, uint64_t now_ms)
{
    memset(sample, 0, sizeof(*sample));
    sample->timestamp_ms = now_ms;
    sample->battery_v = sensor_service_read_battery_v();
    sample->voc_index = 100.0f;
    sample->pressure_pa = 101325.0f;
    sample->temperature_c = 25.0f;

    if (ctx->has_latest_sample && ctx->latest_sample.valid) {
        sample->battery_v = ctx->latest_sample.battery_v;
        sample->voc_index = ctx->latest_sample.voc_index;
        sample->pressure_pa = ctx->latest_sample.pressure_pa;
        sample->temperature_c = ctx->latest_sample.temperature_c;
    }
}

static esp_err_t gate9_read_selected_sample(app_gate_ctx_t *ctx, uint64_t now_ms, sensor_sample_t *sample)
{
    if (sample == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const bool expect_sgp40 = gate9_expect_sgp40();
    const bool expect_bmp280 = gate9_expect_bmp280();

    if (expect_sgp40 && expect_bmp280) {
        return sensor_service_read(sample);
    }

    gate9_seed_sample_defaults(ctx, sample, now_ms);

    if (expect_sgp40) {
        float voc_index = 0.0f;
        esp_err_t err = sensor_service_read_voc_only(&voc_index);
        if (err != ESP_OK) {
            return err;
        }
        sample->voc_index = voc_index;
    }

    if (expect_bmp280) {
        float pressure_pa = 0.0f;
        float temperature_c = 0.0f;
        esp_err_t err = sensor_service_read_bmp280(&pressure_pa, &temperature_c);
        if (err != ESP_OK) {
            return err;
        }
        sample->pressure_pa = pressure_pa;
        sample->temperature_c = temperature_c;
    }

    sample->valid = gate9_sample_in_range(sample);
    return sample->valid ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

static bool gate_valid(app_gate_t gate)
{
    switch (gate) {
    case APP_GATE_0_ENV:
    case APP_GATE_1_HEARTBEAT:
    case APP_GATE_2_DISPLAY_SMOKE:
    case APP_GATE_2P1_I2C_SMOKE:
    case APP_GATE_3_I2C_PRESENCE:
    case APP_GATE_4_SENSOR_PIPELINE:
    case APP_GATE_5_PAYLOAD_V1:
    case APP_GATE_6_LORAWAN_JOIN_UPLINK:
    case APP_GATE_7_RELIABILITY_BUFFER:
    case APP_GATE_8_FUOTA_SCAFFOLD:
    case APP_GATE_9_LIVE_PUBLISH:
        return true;
    default:
        return false;
    }
}

const char *app_gate_name(app_gate_t gate)
{
    switch (gate) {
    case APP_GATE_0_ENV:
        return "env";
    case APP_GATE_1_HEARTBEAT:
        return "heartbeat";
    case APP_GATE_2_DISPLAY_SMOKE:
        return "display_smoke";
    case APP_GATE_2P1_I2C_SMOKE:
        return "i2c_smoke";
    case APP_GATE_3_I2C_PRESENCE:
        return "i2c_presence";
    case APP_GATE_4_SENSOR_PIPELINE:
        return "sensor_pipeline";
    case APP_GATE_5_PAYLOAD_V1:
        return "payload_v1";
    case APP_GATE_6_LORAWAN_JOIN_UPLINK:
        return "lorawan_join_uplink";
    case APP_GATE_7_RELIABILITY_BUFFER:
        return "reliability_buffer";
    case APP_GATE_8_FUOTA_SCAFFOLD:
        return "fuota_scaffold";
    case APP_GATE_9_LIVE_PUBLISH:
        return "live_publish";
    default:
        return "unknown";
    }
}

static const char *app_gate_log_id(app_gate_t gate)
{
    switch (gate) {
    case APP_GATE_0_ENV:
        return "0";
    case APP_GATE_1_HEARTBEAT:
        return "1";
    case APP_GATE_2_DISPLAY_SMOKE:
        return "2";
    case APP_GATE_2P1_I2C_SMOKE:
        return "2.1";
    case APP_GATE_3_I2C_PRESENCE:
        return "3";
    case APP_GATE_4_SENSOR_PIPELINE:
        return "4";
    case APP_GATE_5_PAYLOAD_V1:
        return "5";
    case APP_GATE_6_LORAWAN_JOIN_UPLINK:
        return "6";
    case APP_GATE_7_RELIABILITY_BUFFER:
        return "7";
    case APP_GATE_8_FUOTA_SCAFFOLD:
        return "8";
    case APP_GATE_9_LIVE_PUBLISH:
        return "9";
    default:
        return "unknown";
    }
}

static void gate_log_progress(app_gate_ctx_t *ctx, uint64_t now_ms, const char *fmt, ...)
{
    if (now_ms - ctx->last_progress_log_ms < APP_GATE_PROGRESS_LOG_MS) {
        return;
    }

    char line[192] = {0};
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    ESP_LOGI(TAG, "gate=%s name=%s %s", app_gate_log_id(ctx->selected), app_gate_name(ctx->selected), line);
    ctx->last_progress_log_ms = now_ms;
}

static void gate_mark_pass(app_gate_ctx_t *ctx, const char *fmt, ...)
{
    if (ctx->halted) {
        return;
    }

    char line[192] = {0};
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    ctx->pass = true;
    ctx->halted = true;
    ESP_LOGI(TAG, "result=PASS gate=%s name=%s %s", app_gate_log_id(ctx->selected), app_gate_name(ctx->selected), line);
    ESP_LOGI(TAG, "handshake=STOP_AFTER_PASS gate=%s action=change_CONFIG_APP_GATE_and_reflash", app_gate_log_id(ctx->selected));
}

static void gate_log_fail(app_gate_ctx_t *ctx, uint64_t now_ms, const char *fmt, ...)
{
    if (now_ms - ctx->last_progress_log_ms < APP_GATE_PROGRESS_LOG_MS) {
        return;
    }

    char line[192] = {0};
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    ctx->failures++;
    ESP_LOGW(
        TAG,
        "result=FAIL gate=%s name=%s failures=%lu %s",
        app_gate_log_id(ctx->selected),
        app_gate_name(ctx->selected),
        (unsigned long)ctx->failures,
        line);
    ctx->last_progress_log_ms = now_ms;
}

static void gate_tick_0(app_gate_ctx_t *ctx)
{
    gate_mark_pass(
        ctx,
        "toolchain_sanity=1 fw_build_date=%s fw_build_time=%s idf=%s",
        __DATE__,
        __TIME__,
        esp_get_idf_version());
}

static void gate_tick_1(app_gate_ctx_t *ctx, uint64_t now_ms)
{
    const uint32_t toggles = heartbeat_get_toggle_count();
    gate_log_progress(ctx, now_ms, "heartbeat_toggle_count=%lu", (unsigned long)toggles);

    if (!heartbeat_is_running()) {
        gate_log_fail(ctx, now_ms, "heartbeat_not_running");
        return;
    }

    if ((now_ms - ctx->start_ms) >= 3000 && toggles >= 6) {
        gate_mark_pass(ctx, "heartbeat_ok toggles=%lu", (unsigned long)toggles);
    }
}

static void gate_tick_2(app_gate_ctx_t *ctx)
{
    esp_err_t err = display_service_spi_bus_check();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gate=2 display_spi_check_fail err=%s", esp_err_to_name(err));
        return;
    }

    err = display_service_render_hello_world();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gate=2 display_smoke_fail err=%s", esp_err_to_name(err));
        return;
    }
    gate_mark_pass(ctx, "display_smoke_ok mode=hello_world spi_check=1");
}

static void gate_tick_2p1(app_gate_ctx_t *ctx, uint64_t now_ms)
{
    if (!ctx->i2c_scan_done) {
        i2c_scan_summary_t scan = {0};
        i2c_bus_scan(&scan);
        ctx->i2c_scan_done = true;
    }

    bool sgp40 = false;
    bool bmp280 = false;
    const bool expect_sgp40 = gate2p1_expect_sgp40();
    const bool expect_bmp280 = gate2p1_expect_bmp280();
    esp_err_t err_sgp40 = i2c_bus_probe((uint8_t)CONFIG_APP_SGP40_ADDR, &sgp40);
    esp_err_t err_bmp280 = i2c_bus_probe((uint8_t)CONFIG_APP_BMP280_ADDR, &bmp280);

    if (err_sgp40 != ESP_OK || err_bmp280 != ESP_OK) {
        gate_log_fail(
            ctx,
            now_ms,
            "i2c_smoke_probe_error expected=%d sgp40_err=%s bmp280_err=%s",
            CONFIG_APP_GATE2P1_EXPECTED_DEVICES,
            esp_err_to_name(err_sgp40),
            esp_err_to_name(err_bmp280));
        return;
    }

    if (expect_sgp40 && !sgp40) {
        ctx->gate3_missing_sgp40++;
    }
    if (expect_bmp280 && !bmp280) {
        ctx->gate3_missing_bmp280++;
    }

    gate_log_progress(
        ctx,
        now_ms,
        "i2c_smoke expected=%d sgp40=%d bmp280=%d miss_sgp40=%lu miss_bmp280=%lu",
        CONFIG_APP_GATE2P1_EXPECTED_DEVICES,
        sgp40,
        bmp280,
        (unsigned long)ctx->gate3_missing_sgp40,
        (unsigned long)ctx->gate3_missing_bmp280);

    const bool pass_sgp40 = (!expect_sgp40 || sgp40);
    const bool pass_bmp280 = (!expect_bmp280 || bmp280);
    if (pass_sgp40 && pass_bmp280) {
        gate_mark_pass(
            ctx,
            "i2c_smoke_ok alias=2.1 expected=%d sgp40_addr=0x%02X bmp280_addr=0x%02X",
            CONFIG_APP_GATE2P1_EXPECTED_DEVICES,
            CONFIG_APP_SGP40_ADDR,
            CONFIG_APP_BMP280_ADDR);
    }
}

static void gate_tick_3(app_gate_ctx_t *ctx, uint64_t now_ms)
{
    if (!ctx->i2c_scan_done) {
        i2c_scan_summary_t scan = {0};
        i2c_bus_scan(&scan);
        ctx->i2c_scan_done = true;
    }

    bool sgp40 = false;
    bool bmp280 = false;
    const bool expect_sgp40 = gate3_expect_sgp40();
    const bool expect_bmp280 = gate3_expect_bmp280();
    esp_err_t err_sgp40 = i2c_bus_probe((uint8_t)CONFIG_APP_SGP40_ADDR, &sgp40);
    esp_err_t err_bmp280 = i2c_bus_probe((uint8_t)CONFIG_APP_BMP280_ADDR, &bmp280);

    if (err_sgp40 != ESP_OK || err_bmp280 != ESP_OK) {
        gate_log_fail(
            ctx,
            now_ms,
            "probe_error sgp40_err=%s bmp280_err=%s",
            esp_err_to_name(err_sgp40),
            esp_err_to_name(err_bmp280));
        return;
    }

    if (expect_sgp40 && !sgp40) {
        ctx->gate3_missing_sgp40++;
    }
    if (expect_bmp280 && !bmp280) {
        ctx->gate3_missing_bmp280++;
    }

    gate_log_progress(
        ctx,
        now_ms,
        "i2c_presence expected=%d sgp40=%d bmp280=%d miss_sgp40=%lu miss_bmp280=%lu",
        CONFIG_APP_GATE3_EXPECTED_DEVICES,
        sgp40,
        bmp280,
        (unsigned long)ctx->gate3_missing_sgp40,
        (unsigned long)ctx->gate3_missing_bmp280);

    const bool pass_sgp40 = (!expect_sgp40 || sgp40);
    const bool pass_bmp280 = (!expect_bmp280 || bmp280);
    if (pass_sgp40 && pass_bmp280) {
        gate_mark_pass(
            ctx,
            "presence_ok expected=%d sgp40_addr=0x%02X bmp280_addr=0x%02X",
            CONFIG_APP_GATE3_EXPECTED_DEVICES,
            CONFIG_APP_SGP40_ADDR,
            CONFIG_APP_BMP280_ADDR);
    }
}

static void gate_tick_4(app_gate_ctx_t *ctx, uint64_t now_ms)
{
    const bool expect_sgp40 = gate4_expect_sgp40();
    const bool expect_bmp280 = gate4_expect_bmp280();

    if (expect_sgp40) {
        sensor_identity_t id_sgp40 = {0};
        esp_err_t err = sensor_service_read_sgp40_identity(&id_sgp40);
        if (err != ESP_OK || !id_sgp40.sgp40_serial_ok) {
            gate_log_fail(
                ctx,
                now_ms,
                "identity_fail expected=%d sgp40_err=%s sgp40_serial_ok=%d",
                CONFIG_APP_GATE4_EXPECTED_DEVICES,
                esp_err_to_name(err),
                id_sgp40.sgp40_serial_ok);
            return;
        }
    }

    if (expect_bmp280) {
        sensor_identity_t id_bmp280 = {0};
        esp_err_t err = sensor_service_read_bmp280_identity(&id_bmp280);
        if (err != ESP_OK || !id_bmp280.bmp280_chip_id_ok) {
            gate_log_fail(
                ctx,
                now_ms,
                "identity_fail expected=%d bmp280_err=%s bmp280_chip_id_ok=%d bmp280_chip_id=0x%02X",
                CONFIG_APP_GATE4_EXPECTED_DEVICES,
                esp_err_to_name(err),
                id_bmp280.bmp280_chip_id_ok,
                id_bmp280.bmp280_chip_id);
            return;
        }
    }

    ctx->gate3_identity_ok = true;

    if (expect_sgp40 && expect_bmp280) {
        sensor_sample_t sample = {0};
        esp_err_t err = sensor_service_read(&sample);
        if (err != ESP_OK || !sample.valid) {
            ctx->sensor_fail++;
            gate_log_fail(ctx, now_ms, "sensor_read_fail expected=3 err=%s sensor_fail=%lu", esp_err_to_name(err), (unsigned long)ctx->sensor_fail);
            return;
        }

        const bool in_range = (sample.voc_index >= 0.0f && sample.voc_index <= 500.0f && sample.pressure_pa >= 80000.0f && sample.pressure_pa <= 120000.0f && sample.temperature_c >= -40.0f && sample.temperature_c <= 85.0f);
        if (!in_range) {
            ctx->sensor_fail++;
            gate_log_fail(
                ctx,
                now_ms,
                "range_fail expected=3 voc=%.2f pressure=%.1f temp=%.2f",
                sample.voc_index,
                sample.pressure_pa,
                sample.temperature_c);
            return;
        }

        ctx->sensor_ok++;
        ctx->latest_sample = sample;
        ctx->has_latest_sample = true;

        gate_log_progress(
            ctx,
            now_ms,
            "sensor_ok_count=%lu expected=3 voc=%.2f pressure=%.1f temp=%.2f",
            (unsigned long)ctx->sensor_ok,
            sample.voc_index,
            sample.pressure_pa,
            sample.temperature_c);
    } else if (expect_sgp40) {
        float voc_index = 0.0f;
        esp_err_t err = sensor_service_read_voc_only(&voc_index);
        if (err != ESP_OK) {
            ctx->sensor_fail++;
            gate_log_fail(ctx, now_ms, "voc_only_fail expected=1 err=%s sensor_fail=%lu", esp_err_to_name(err), (unsigned long)ctx->sensor_fail);
            return;
        }
        if (voc_index < 0.0f || voc_index > 500.0f) {
            ctx->sensor_fail++;
            gate_log_fail(ctx, now_ms, "voc_range_fail expected=1 voc=%.2f", voc_index);
            return;
        }
        ctx->sensor_ok++;
        gate_log_progress(ctx, now_ms, "sensor_ok_count=%lu expected=1 voc=%.2f", (unsigned long)ctx->sensor_ok, voc_index);
    } else {
        float pressure_pa = 0.0f;
        float temperature_c = 0.0f;
        esp_err_t err = sensor_service_read_bmp280(&pressure_pa, &temperature_c);
        if (err != ESP_OK) {
            ctx->sensor_fail++;
            gate_log_fail(ctx, now_ms, "bmp280_read_fail expected=2 err=%s sensor_fail=%lu", esp_err_to_name(err), (unsigned long)ctx->sensor_fail);
            return;
        }
        if (pressure_pa < 80000.0f || pressure_pa > 120000.0f || temperature_c < -40.0f || temperature_c > 85.0f) {
            ctx->sensor_fail++;
            gate_log_fail(ctx, now_ms, "bmp280_range_fail expected=2 pressure=%.1f temp=%.2f", pressure_pa, temperature_c);
            return;
        }
        ctx->sensor_ok++;
        gate_log_progress(
            ctx,
            now_ms,
            "sensor_ok_count=%lu expected=2 pressure=%.1fPa temp=%.2fC",
            (unsigned long)ctx->sensor_ok,
            pressure_pa,
            temperature_c);
    }

    if (ctx->gate3_identity_ok && ctx->sensor_ok >= 3) {
        gate_mark_pass(
            ctx,
            "sensor_pipeline_ok expected=%d identity=1 sensor_ok=%lu sensor_fail=%lu",
            CONFIG_APP_GATE4_EXPECTED_DEVICES,
            (unsigned long)ctx->sensor_ok,
            (unsigned long)ctx->sensor_fail);
    }
}

static void gate_tick_5(app_gate_ctx_t *ctx, uint64_t now_ms)
{
    const sensor_sample_t vector = {
        .voc_index = 100.0f,
        .pressure_pa = 100900.0f,
        .temperature_c = 29.0f,
        .battery_v = 4.0f,
        .valid = true,
        .timestamp_ms = now_ms,
    };

    static const uint8_t expected[APP_PAYLOAD_V1_LEN] = {0x01, 0x27, 0x10, 0x00, 0x01, 0x8A, 0x24, 0x0B, 0x54, 0x0F, 0xA0, 0x01};

    uint8_t payload[APP_PAYLOAD_V1_LEN] = {0};
    const size_t len = app_payload_encode_v1(&vector, payload, sizeof(payload));
    if (len != APP_PAYLOAD_V1_LEN) {
        ctx->payload_fail++;
        gate_log_fail(ctx, now_ms, "payload_len_fail len=%lu", (unsigned long)len);
        return;
    }

    if (memcmp(payload, expected, APP_PAYLOAD_V1_LEN) != 0) {
        char hex[APP_PAYLOAD_V1_LEN * 3] = {0};
        app_payload_hex(payload, APP_PAYLOAD_V1_LEN, hex, sizeof(hex));
        ctx->payload_fail++;
        gate_log_fail(ctx, now_ms, "payload_vector_mismatch hex=%s", hex);
        return;
    }

    memcpy(ctx->latest_payload, payload, APP_PAYLOAD_V1_LEN);
    ctx->latest_payload_len = APP_PAYLOAD_V1_LEN;
    ctx->payload_ok++;

    char hex[APP_PAYLOAD_V1_LEN * 3] = {0};
    app_payload_hex(payload, APP_PAYLOAD_V1_LEN, hex, sizeof(hex));
    ESP_LOGI(TAG, "gate=5 payload_schema=1 payload_hex=%s", hex);
    gate_mark_pass(ctx, "payload_encode_ok schema=1 bytes=%d", APP_PAYLOAD_V1_LEN);
}

static sensor_sample_t nominal_sample(uint64_t now_ms)
{
    const sensor_sample_t s = {
        .voc_index = 101.1f,
        .pressure_pa = 100950.0f,
        .temperature_c = 28.9f,
        .battery_v = 3.95f,
        .valid = true,
        .timestamp_ms = now_ms,
    };
    return s;
}

static void gate_tick_6(app_gate_ctx_t *ctx, uint64_t now_ms)
{
    lorawan_service_process(now_ms);

    if (!ctx->has_latest_sample) {
        ctx->latest_sample = nominal_sample(now_ms);
        ctx->has_latest_sample = true;
    }

    if (!ctx->gate5_uplink_done && lorawan_service_is_joined()) {
        ctx->latest_payload_len = app_payload_encode_v1(&ctx->latest_sample, ctx->latest_payload, sizeof(ctx->latest_payload));
        esp_err_t err = lorawan_service_send(ctx->latest_payload, ctx->latest_payload_len);
        if (err == ESP_OK) {
            ctx->gate5_uplink_done = true;
        } else {
            gate_log_fail(ctx, now_ms, "uplink_fail err=%s", esp_err_to_name(err));
        }
    }

    lorawan_metrics_t m = {0};
    lorawan_service_get_metrics(&m);
    gate_log_progress(
        ctx,
        now_ms,
        "join_attempts=%lu joined=%d uplink_ok=%lu backend_active=%d retries=%lu",
        (unsigned long)m.join_attempts,
        lorawan_service_is_joined(),
        (unsigned long)m.uplink_ok,
        m.backend_active,
        (unsigned long)m.retries);

    if (lorawan_service_is_joined() && m.uplink_ok >= 1) {
        gate_mark_pass(
            ctx,
            "joined=1 uplink_done=1 join_attempts=%lu backend_active=%d",
            (unsigned long)m.join_attempts,
            m.backend_active);
    }
}

static void gate_tick_7(app_gate_ctx_t *ctx, uint64_t now_ms)
{
    if (!ctx->gate6_backend_enabled && (now_ms - ctx->start_ms) >= (uint64_t)CONFIG_APP_GATE6_BACKEND_ENABLE_DELAY_MS) {
        ESP_ERROR_CHECK(lorawan_service_set_backend_active(true));
        ctx->gate6_backend_enabled = true;
    }

    lorawan_service_process(now_ms);

    if (!ctx->gate6_buffer_valid) {
        const sensor_sample_t s = nominal_sample(now_ms);
        ctx->latest_payload_len = app_payload_encode_v1(&s, ctx->latest_payload, sizeof(ctx->latest_payload));
        ctx->gate6_buffer_valid = (ctx->latest_payload_len == APP_PAYLOAD_V1_LEN);
        if (ctx->gate6_buffer_valid) {
            ctx->buffered_count++;
            ESP_LOGI(TAG, "gate=7 buffer_store bytes=%lu", (unsigned long)ctx->latest_payload_len);
        }
    }

    if (ctx->gate6_buffer_valid && lorawan_service_is_joined()) {
        esp_err_t err = lorawan_service_send(ctx->latest_payload, ctx->latest_payload_len);
        if (err == ESP_OK) {
            ctx->gate6_buffer_valid = false;
            ctx->flushed_count++;
            ESP_LOGI(TAG, "gate=7 buffer_flush_ok flushed_count=%lu", (unsigned long)ctx->flushed_count);
        }
    }

    lorawan_metrics_t m = {0};
    lorawan_service_get_metrics(&m);
    gate_log_progress(
        ctx,
        now_ms,
        "buffered=%lu flushed=%lu joined=%d join_attempts=%lu retries=%lu",
        (unsigned long)ctx->buffered_count,
        (unsigned long)ctx->flushed_count,
        lorawan_service_is_joined(),
        (unsigned long)m.join_attempts,
        (unsigned long)m.retries);

    if (lorawan_service_is_joined() && ctx->flushed_count >= 1) {
        gate_mark_pass(
            ctx,
            "reliability_ok joined=1 buffered=%lu flushed=%lu uplink_ok=%lu",
            (unsigned long)ctx->buffered_count,
            (unsigned long)ctx->flushed_count,
            (unsigned long)m.uplink_ok);
    }
}

static void gate_tick_8(app_gate_ctx_t *ctx)
{
    ESP_LOGI("FUOTA", "manifest=placeholder-v1 region=AS923-1 mode=emergency-only");
    ESP_LOGI("FUOTA", "multicast_hook=ready fragment_hook=ready rollback_policy=hard_required");
    ESP_LOGI("FUOTA", "version_payload=01 00 00 00 downgrade_block=1 max_downtime_min=30");
    gate_mark_pass(ctx, "fuota_scaffold_ready hooks=1 rollback=1");
}

static void gate_tick_9(app_gate_ctx_t *ctx, uint64_t now_ms)
{
    lorawan_service_process(now_ms);

    if ((now_ms - ctx->last_sample_ms) >= (uint64_t)CONFIG_APP_GATE_SAMPLE_PERIOD_MS) {
        sensor_sample_t sample = {0};
        esp_err_t err = gate9_read_selected_sample(ctx, now_ms, &sample);
        if (err == ESP_OK && sample.valid) {
            ctx->sensor_ok++;
            ctx->latest_sample = sample;
            ctx->has_latest_sample = true;

            if (display_service_should_refresh(
                    ctx->has_last_rendered ? &ctx->last_rendered : NULL,
                    &ctx->latest_sample,
                    now_ms,
                    ctx->last_refresh_ms)) {
                if (display_service_render(&ctx->latest_sample) == ESP_OK) {
                    ctx->display_updates++;
                    ctx->last_refresh_ms = now_ms;
                    ctx->last_rendered = ctx->latest_sample;
                    ctx->has_last_rendered = true;
                }
            }
        } else {
            ctx->sensor_fail++;
            gate_log_fail(
                ctx,
                now_ms,
                "sample_read_fail expected=%d err=%s sensor_fail=%lu",
                CONFIG_APP_GATE9_EXPECTED_DEVICES,
                esp_err_to_name(err),
                (unsigned long)ctx->sensor_fail);
        }
        ctx->last_sample_ms = now_ms;
    }

    if (ctx->has_latest_sample && (now_ms - ctx->last_uplink_ms) >= (uint64_t)CONFIG_APP_GATE_UPLINK_PERIOD_MS) {
        ctx->latest_payload_len = app_payload_encode_v1(&ctx->latest_sample, ctx->latest_payload, sizeof(ctx->latest_payload));
        if (ctx->latest_payload_len == APP_PAYLOAD_V1_LEN) {
            (void)lorawan_service_send(ctx->latest_payload, ctx->latest_payload_len);
        }
        ctx->last_uplink_ms = now_ms;
    }

    lorawan_metrics_t m = {0};
    lorawan_service_get_metrics(&m);
    gate_log_progress(
        ctx,
        now_ms,
        "sensor_ok=%lu display_updates=%lu joined=%d uplink_ok=%lu expected=%d",
        (unsigned long)ctx->sensor_ok,
        (unsigned long)ctx->display_updates,
        lorawan_service_is_joined(),
        (unsigned long)m.uplink_ok,
        CONFIG_APP_GATE9_EXPECTED_DEVICES);

    if (ctx->sensor_ok >= 2 && ctx->display_updates >= 1 && m.uplink_ok >= 1) {
        gate_mark_pass(
            ctx,
            "live_publish_ok expected=%d sensor_ok=%lu display_updates=%lu uplink_ok=%lu",
            CONFIG_APP_GATE9_EXPECTED_DEVICES,
            (unsigned long)ctx->sensor_ok,
            (unsigned long)ctx->display_updates,
            (unsigned long)m.uplink_ok);
    }
}

static bool gate_requires_i2c(app_gate_t gate)
{
    switch (gate) {
    case APP_GATE_2P1_I2C_SMOKE:
    case APP_GATE_3_I2C_PRESENCE:
    case APP_GATE_4_SENSOR_PIPELINE:
    case APP_GATE_9_LIVE_PUBLISH:
        return true;
    default:
        return false;
    }
}

static bool gate_requires_sensor_init(app_gate_t gate)
{
    switch (gate) {
    case APP_GATE_4_SENSOR_PIPELINE:
    case APP_GATE_9_LIVE_PUBLISH:
        return true;
    default:
        return false;
    }
}

static bool gate_requires_display_init(app_gate_t gate)
{
    return (gate == APP_GATE_2_DISPLAY_SMOKE || gate == APP_GATE_9_LIVE_PUBLISH);
}

static bool gate_requires_lorawan_init(app_gate_t gate)
{
    switch (gate) {
    case APP_GATE_6_LORAWAN_JOIN_UPLINK:
    case APP_GATE_7_RELIABILITY_BUFFER:
    case APP_GATE_9_LIVE_PUBLISH:
        return true;
    default:
        return false;
    }
}

esp_err_t app_gate_init(app_gate_ctx_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(ctx, 0, sizeof(*ctx));

    int selected = CONFIG_APP_GATE;
#if CONFIG_APP_GATE_ID_SCHEME_LEGACY
    selected = app_gate_translate_legacy_id(CONFIG_APP_GATE_LEGACY);
    ESP_LOGI(TAG, "gate_id_scheme=legacy input=%d resolved=%d", CONFIG_APP_GATE_LEGACY, selected);
#else
    ESP_LOGI(TAG, "gate_id_scheme=new selected=%d", selected);
#endif

    if (selected < 0) {
        ESP_LOGE(TAG, "invalid_gate gate=%d allowed=0..9|21(2.1)", selected);
        return ESP_ERR_INVALID_ARG;
    }

    ctx->selected = (app_gate_t)selected;
    if (!gate_valid(ctx->selected)) {
        ESP_LOGE(TAG, "invalid_gate gate=%d allowed=0..9|21(2.1)", (int)ctx->selected);
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(
        TAG,
        "boot board=RAK3312 gate=%s gate_raw=%d name=%s idf=%s build=%s %s",
        app_gate_log_id(ctx->selected),
        (int)ctx->selected,
        app_gate_name(ctx->selected),
        esp_get_idf_version(),
        __DATE__,
        __TIME__);

    if (ctx->selected == APP_GATE_1_HEARTBEAT) {
        ESP_RETURN_ON_ERROR(heartbeat_start(), TAG, "heartbeat_start_failed");
    } else {
        ESP_LOGI(TAG, "heartbeat_skip gate=%s reason=non_heartbeat_gate", app_gate_log_id(ctx->selected));
    }

    if (gate_requires_i2c(ctx->selected)) {
        ESP_RETURN_ON_ERROR(i2c_bus_init(), TAG, "i2c_init_failed");
    }

    if (gate_requires_sensor_init(ctx->selected)) {
        ESP_RETURN_ON_ERROR(sensor_service_init(), TAG, "sensor_init_failed");

        if (ctx->selected == APP_GATE_4_SENSOR_PIPELINE) {
            if (gate4_expect_sgp40() && !sensor_service_is_sgp40_present()) {
                ESP_LOGE(TAG, "gate4_missing_required_sgp40 expected_devices=%d", CONFIG_APP_GATE4_EXPECTED_DEVICES);
                return ESP_ERR_NOT_FOUND;
            }
            if (gate4_expect_bmp280() && !sensor_service_is_bmp280_present()) {
                ESP_LOGE(TAG, "gate4_missing_required_bmp280 expected_devices=%d", CONFIG_APP_GATE4_EXPECTED_DEVICES);
                return ESP_ERR_NOT_FOUND;
            }
        }

        if (ctx->selected == APP_GATE_9_LIVE_PUBLISH) {
            if (gate9_expect_sgp40() && !sensor_service_is_sgp40_present()) {
                ESP_LOGE(TAG, "gate9_missing_required_sgp40 expected_devices=%d", CONFIG_APP_GATE9_EXPECTED_DEVICES);
                return ESP_ERR_NOT_FOUND;
            }
            if (gate9_expect_bmp280() && !sensor_service_is_bmp280_present()) {
                ESP_LOGE(TAG, "gate9_missing_required_bmp280 expected_devices=%d", CONFIG_APP_GATE9_EXPECTED_DEVICES);
                return ESP_ERR_NOT_FOUND;
            }
        }
    }

    if (gate_requires_display_init(ctx->selected)) {
        ESP_RETURN_ON_ERROR(display_service_init(), TAG, "display_init_failed");
    }

    if (gate_requires_lorawan_init(ctx->selected)) {
        ESP_RETURN_ON_ERROR(lorawan_service_init(), TAG, "lorawan_init_failed");
        if (ctx->selected == APP_GATE_7_RELIABILITY_BUFFER) {
            ESP_RETURN_ON_ERROR(lorawan_service_set_backend_active(false), TAG, "lorawan_backend_disable_failed");
        }
    }

    return ESP_OK;
}

void app_gate_tick(app_gate_ctx_t *ctx, uint64_t now_ms)
{
    if (ctx == NULL) {
        return;
    }

    if (ctx->start_ms == 0) {
        ctx->start_ms = now_ms;
        ctx->last_progress_log_ms = now_ms;
        ctx->last_idle_log_ms = now_ms;
        ctx->last_sample_ms = now_ms;
        ctx->last_uplink_ms = now_ms;
        ctx->last_refresh_ms = now_ms;
    }

    if (ctx->halted) {
        if ((now_ms - ctx->last_idle_log_ms) >= ((uint64_t)CONFIG_APP_GATE_IDLE_LOG_SEC * 1000ULL)) {
            ESP_LOGI(
                TAG,
                "gate=%s halted_after_pass=1 next_action=set_CONFIG_APP_GATE_and_reflash",
                app_gate_log_id(ctx->selected));
            ctx->last_idle_log_ms = now_ms;
        }
        return;
    }

    switch (ctx->selected) {
    case APP_GATE_0_ENV:
        gate_tick_0(ctx);
        break;
    case APP_GATE_1_HEARTBEAT:
        gate_tick_1(ctx, now_ms);
        break;
    case APP_GATE_2_DISPLAY_SMOKE:
        gate_tick_2(ctx);
        break;
    case APP_GATE_2P1_I2C_SMOKE:
        gate_tick_2p1(ctx, now_ms);
        break;
    case APP_GATE_3_I2C_PRESENCE:
        gate_tick_3(ctx, now_ms);
        break;
    case APP_GATE_4_SENSOR_PIPELINE:
        gate_tick_4(ctx, now_ms);
        break;
    case APP_GATE_5_PAYLOAD_V1:
        gate_tick_5(ctx, now_ms);
        break;
    case APP_GATE_6_LORAWAN_JOIN_UPLINK:
        gate_tick_6(ctx, now_ms);
        break;
    case APP_GATE_7_RELIABILITY_BUFFER:
        gate_tick_7(ctx, now_ms);
        break;
    case APP_GATE_8_FUOTA_SCAFFOLD:
        gate_tick_8(ctx);
        break;
    case APP_GATE_9_LIVE_PUBLISH:
        gate_tick_9(ctx, now_ms);
        break;
    default:
        gate_log_fail(ctx, now_ms, "invalid_gate_runtime");
        break;
    }
}

bool app_gate_is_halted(const app_gate_ctx_t *ctx)
{
    return ctx != NULL && ctx->halted;
}
