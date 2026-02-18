#include "display_service.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "display_epd_ssd1680.h"
#include "display_font_5x7.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "DISPLAY";

#define PANEL_W_250 250
#define PANEL_H_122 122
#define PANEL_W_212 212
#define PANEL_H_104 104
#define PANEL_NATIVE_W_250 122
#define PANEL_NATIVE_H_250 250
#define PANEL_NATIVE_W_212 104
#define PANEL_NATIVE_H_212 212
#define PANEL_MAX_BYTES (((PANEL_NATIVE_W_250 + 7) / 8) * PANEL_NATIVE_H_250)

#if CONFIG_APP_DISPLAY_LANDSCAPE
#define DISPLAY_ORIENTATION "landscape"
#else
#define DISPLAY_ORIENTATION "portrait"
#endif

#if CONFIG_APP_DISPLAY_PARTIAL_UPDATES
#define DISPLAY_PARTIAL_UPDATES_ENABLED true
#else
#define DISPLAY_PARTIAL_UPDATES_ENABLED false
#endif

#if CONFIG_APP_DISPLAY_BUSY_ACTIVE_HIGH
#define DISPLAY_BUSY_ACTIVE_HIGH true
#else
#define DISPLAY_BUSY_ACTIVE_HIGH false
#endif

#if CONFIG_APP_DISPLAY_PWR_ACTIVE_HIGH
#define DISPLAY_PWR_ACTIVE_HIGH true
#define DISPLAY_PWR_ENABLE_LEVEL 1
#else
#define DISPLAY_PWR_ACTIVE_HIGH false
#define DISPLAY_PWR_ENABLE_LEVEL 0
#endif

static bool s_init_done;
static int s_width;
static int s_height;
static int s_native_width;
static int s_native_height;
static uint8_t s_framebuffer_bw[PANEL_MAX_BYTES];
static uint8_t s_framebuffer_red[PANEL_MAX_BYTES];

typedef enum {
    PIXEL_BLACK = 0,
    PIXEL_RED = 1,
} pixel_color_t;

static void panel_dimensions(int *width, int *height, int *native_width, int *native_height)
{
#if CONFIG_APP_DISPLAY_PANEL_PROFILE_SSD1680_212X104
    *width = PANEL_W_212;
    *height = PANEL_H_104;
    *native_width = PANEL_NATIVE_W_212;
    *native_height = PANEL_NATIVE_H_212;
#else
    *width = PANEL_W_250;
    *height = PANEL_H_122;
    *native_width = PANEL_NATIVE_W_250;
    *native_height = PANEL_NATIVE_H_250;
#endif
}

static size_t framebuffer_len(void)
{
    const size_t row_bytes = (size_t)((s_native_width + 7) / 8);
    return row_bytes * (size_t)s_native_height;
}

static void framebuffer_clear_white(void)
{
    memset(s_framebuffer_bw, 0xFF, framebuffer_len());
    memset(s_framebuffer_red, 0x00, framebuffer_len());
}

static void framebuffer_set_plane_bit(uint8_t *buffer, int nx, int ny, bool white)
{
    const int bytes_per_row = (s_native_width + 7) / 8;
    const int byte_index = (ny * bytes_per_row) + (nx / 8);
    const uint8_t bit = (uint8_t)(1U << (7 - (nx % 8)));

    if (white) {
        buffer[byte_index] |= bit;
    } else {
        buffer[byte_index] &= (uint8_t)~bit;
    }
}

static void framebuffer_set_pixel(int x, int y, pixel_color_t color, bool on)
{
    if (x < 0 || y < 0 || x >= s_width || y >= s_height) {
        return;
    }

    int nx = x;
    int ny = y;
#if CONFIG_APP_DISPLAY_LANDSCAPE
    nx = y;
    ny = (s_native_height - 1) - x;
#endif

    if (nx < 0 || ny < 0 || nx >= s_native_width || ny >= s_native_height) {
        return;
    }

    if (!on) {
        // White pixel on tri-color SSD1680: BW=1, RED=0
        framebuffer_set_plane_bit(s_framebuffer_bw, nx, ny, true);
        framebuffer_set_plane_bit(s_framebuffer_red, nx, ny, false);
        return;
    }

    if (color == PIXEL_BLACK) {
        // Black pixel: BW=0, RED=0
        framebuffer_set_plane_bit(s_framebuffer_bw, nx, ny, false);
        framebuffer_set_plane_bit(s_framebuffer_red, nx, ny, false);
    } else {
        // Red pixel: BW=1, RED=1
        framebuffer_set_plane_bit(s_framebuffer_bw, nx, ny, true);
        framebuffer_set_plane_bit(s_framebuffer_red, nx, ny, true);
    }
}

static void framebuffer_draw_char(int x, int y, char c, pixel_color_t color)
{
    uint8_t glyph[5] = {0};
    (void)display_font_5x7_get(c, glyph);

    for (int col = 0; col < 5; col++) {
        for (int row = 0; row < 7; row++) {
            const bool on = ((glyph[col] >> row) & 0x01) != 0;
            framebuffer_set_pixel(x + col, y + row, color, on);
        }
    }
}

static void framebuffer_draw_char_scaled(int x, int y, char c, pixel_color_t color, int scale)
{
    if (scale <= 1) {
        framebuffer_draw_char(x, y, c, color);
        return;
    }

    uint8_t glyph[5] = {0};
    (void)display_font_5x7_get(c, glyph);

    for (int col = 0; col < 5; col++) {
        for (int row = 0; row < 7; row++) {
            const bool on = ((glyph[col] >> row) & 0x01) != 0;
            if (!on) {
                continue;
            }
            for (int sx = 0; sx < scale; sx++) {
                for (int sy = 0; sy < scale; sy++) {
                    framebuffer_set_pixel(x + (col * scale) + sx, y + (row * scale) + sy, color, true);
                }
            }
        }
    }
}

static void framebuffer_draw_text(int x, int y, const char *text, pixel_color_t color)
{
    if (text == NULL) {
        return;
    }

    int cursor_x = x;
    for (size_t i = 0; text[i] != '\0'; i++) {
        framebuffer_draw_char(cursor_x, y, text[i], color);
        cursor_x += 6;
    }
}

static void framebuffer_draw_text_scaled(int x, int y, const char *text, pixel_color_t color, int scale)
{
    if (text == NULL) {
        return;
    }
    if (scale <= 1) {
        framebuffer_draw_text(x, y, text, color);
        return;
    }

    int cursor_x = x;
    for (size_t i = 0; text[i] != '\0'; i++) {
        framebuffer_draw_char_scaled(cursor_x, y, text[i], color, scale);
        cursor_x += (6 * scale);
    }
}

static int framebuffer_text_width_scaled(const char *text, int scale)
{
    if (text == NULL || scale <= 0) {
        return 0;
    }

    int chars = 0;
    while (text[chars] != '\0') {
        chars++;
    }
    return chars * 6 * scale;
}

static void framebuffer_fill_rect(int x, int y, int w, int h, pixel_color_t color)
{
    if (w <= 0 || h <= 0) {
        return;
    }
    for (int yy = y; yy < (y + h); yy++) {
        for (int xx = x; xx < (x + w); xx++) {
            framebuffer_set_pixel(xx, yy, color, true);
        }
    }
}

static void framebuffer_fill_color(pixel_color_t color)
{
    const size_t len = framebuffer_len();
    if (color == PIXEL_BLACK) {
        memset(s_framebuffer_bw, 0x00, len);
        memset(s_framebuffer_red, 0x00, len);
        return;
    }

    if (color == PIXEL_RED) {
        memset(s_framebuffer_bw, 0xFF, len);
        memset(s_framebuffer_red, 0xFF, len);
        return;
    }

    framebuffer_clear_white();
}

static esp_err_t display_flush(void)
{
    return display_epd_ssd1680_draw_frame_dual(s_framebuffer_bw, s_framebuffer_red, framebuffer_len());
}

esp_err_t display_service_init(void)
{
    if (s_init_done) {
        return ESP_OK;
    }

    panel_dimensions(&s_width, &s_height, &s_native_width, &s_native_height);
    display_epd_ssd1680_config_t cfg = {
        .width = s_native_width,
        .height = s_native_height,
        .spi_host = CONFIG_APP_DISPLAY_SPI_HOST,
        .spi_mhz = CONFIG_APP_DISPLAY_SPI_MHZ,
        .pin_mosi = CONFIG_APP_DISPLAY_PIN_MOSI,
        .pin_miso = CONFIG_APP_DISPLAY_PIN_MISO,
        .pin_sclk = CONFIG_APP_DISPLAY_PIN_SCLK,
        .pin_cs = CONFIG_APP_DISPLAY_PIN_CS,
        .pin_dc = CONFIG_APP_DISPLAY_PIN_DC,
        .pin_rst = CONFIG_APP_DISPLAY_PIN_RST,
        .pin_busy = CONFIG_APP_DISPLAY_PIN_BUSY,
        .pin_pwr = CONFIG_APP_DISPLAY_PIN_PWR,
        .xram_offset = CONFIG_APP_DISPLAY_XRAM_OFFSET,
        .partial_updates = DISPLAY_PARTIAL_UPDATES_ENABLED,
        .busy_active_high = DISPLAY_BUSY_ACTIVE_HIGH,
        .pwr_active_high = DISPLAY_PWR_ACTIVE_HIGH,
#if CONFIG_APP_DISPLAY_PWR_INPUT_PULLUP
        .pwr_input_pullup = true,
#else
        .pwr_input_pullup = false,
#endif
    };

    ESP_LOGI(
        TAG,
        "pinmap cs=%d clk=%d mosi=%d miso=%d dc=%d rst=%d busy=%d pwr=%d",
        CONFIG_APP_DISPLAY_PIN_CS,
        CONFIG_APP_DISPLAY_PIN_SCLK,
        CONFIG_APP_DISPLAY_PIN_MOSI,
        CONFIG_APP_DISPLAY_PIN_MISO,
        CONFIG_APP_DISPLAY_PIN_DC,
        CONFIG_APP_DISPLAY_PIN_RST,
        CONFIG_APP_DISPLAY_PIN_BUSY,
        CONFIG_APP_DISPLAY_PIN_PWR);
    ESP_LOGI(
        TAG,
        "power_on wb_io2=%d value=%d mode=%s xram_offset=%d",
        CONFIG_APP_DISPLAY_PIN_PWR,
        DISPLAY_PWR_ENABLE_LEVEL,
#if CONFIG_APP_DISPLAY_PWR_INPUT_PULLUP
        "input_pullup",
#else
        "output",
#endif
        CONFIG_APP_DISPLAY_XRAM_OFFSET);
    esp_err_t err = display_epd_ssd1680_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "init_fail err=%s", esp_err_to_name(err));
        return err;
    }

    framebuffer_clear_white();
    s_init_done = true;
    ESP_LOGI(
        TAG,
        "init_ok panel=ssd1680 res=%dx%d orientation=%s native=%dx%d",
        s_width,
        s_height,
        DISPLAY_ORIENTATION,
        s_native_width,
        s_native_height);
    return ESP_OK;
}

esp_err_t display_service_spi_bus_check(void)
{
    ESP_RETURN_ON_ERROR(display_service_init(), TAG, "display_init_failed");

    display_epd_ssd1680_diag_t diag = {0};
    display_epd_ssd1680_get_diag(&diag);
    const bool spi_ok = (diag.spi_tx_calls > 0 && diag.spi_tx_bytes > 0);
    ESP_LOGI(
        TAG,
        "spi_bus_check spi_ok=%d tx_calls=%" PRIu32 " tx_bytes=%" PRIu32 " reset_busy_seen=%d",
        spi_ok,
        diag.spi_tx_calls,
        diag.spi_tx_bytes,
        diag.reset_busy_pulse_seen);
    if (!spi_ok) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    return ESP_OK;
}

esp_err_t display_service_render_hello_world(void)
{
    ESP_RETURN_ON_ERROR(display_service_init(), TAG, "display_init_failed");

    // Tri-color panels need a clean white cycle before final content to avoid noisy residuals.
    ESP_LOGI(TAG, "hello_world_phase=white_clear");
    framebuffer_clear_white();
    ESP_RETURN_ON_ERROR(display_flush(), TAG, "hello_world_white_flush_failed");

    ESP_LOGI(TAG, "hello_world_phase=black_full");
    framebuffer_fill_color(PIXEL_BLACK);
    ESP_RETURN_ON_ERROR(display_flush(), TAG, "hello_world_black_flush_failed");

    ESP_LOGI(TAG, "hello_world_phase=red_full");
    framebuffer_fill_color(PIXEL_RED);
    ESP_RETURN_ON_ERROR(display_flush(), TAG, "hello_world_red_flush_failed");

    ESP_LOGI(TAG, "hello_world_phase=final_white_base");
    framebuffer_clear_white();
    ESP_RETURN_ON_ERROR(display_flush(), TAG, "hello_world_final_base_failed");

    ESP_LOGI(TAG, "hello_world_phase=final_text_card");
    framebuffer_clear_white();
    framebuffer_fill_rect(0, 0, s_width, 10, PIXEL_BLACK);
    framebuffer_fill_rect(0, (s_height - 10), s_width, 10, PIXEL_RED);
    framebuffer_draw_text_scaled(8, 18, "HELLO", PIXEL_BLACK, 2);
    framebuffer_draw_text_scaled(8, 40, "WORLD", PIXEL_RED, 2);
    ESP_RETURN_ON_ERROR(display_flush(), TAG, "hello_world_text_flush_failed");

    display_epd_ssd1680_diag_t diag = {0};
    display_epd_ssd1680_get_diag(&diag);
    ESP_LOGI(
        TAG,
        "probe_busy reset_seen=%d refresh_seen=%d busy_level=%d",
        diag.reset_busy_pulse_seen,
        diag.refresh_busy_pulse_seen,
        diag.busy_level_snapshot);
#if CONFIG_APP_DISPLAY_REQUIRE_BUSY_PULSE
    if (!diag.refresh_busy_pulse_seen) {
        ESP_LOGE(TAG, "probe_fail no_refresh_busy_pulse");
        return ESP_ERR_INVALID_RESPONSE;
    }
#else
    if (!diag.refresh_busy_pulse_seen) {
        ESP_LOGW(TAG, "probe_warn no_refresh_busy_pulse_using_time_fallback=1");
    }
#endif

    ESP_LOGI(TAG, "hello_world_render_ok");
    return ESP_OK;
}

esp_err_t display_service_render_white_clear(void)
{
    ESP_RETURN_ON_ERROR(display_service_init(), TAG, "display_init_failed");

    ESP_LOGI(TAG, "white_clear_phase=1");
    framebuffer_clear_white();
    ESP_RETURN_ON_ERROR(display_flush(), TAG, "white_clear_flush_failed");

    // Run two cycles for tri-color panels to settle to clean white.
    ESP_LOGI(TAG, "white_clear_phase=2");
    framebuffer_clear_white();
    ESP_RETURN_ON_ERROR(display_flush(), TAG, "white_clear_flush_failed");

    ESP_LOGI(TAG, "white_clear_ok");
    return ESP_OK;
}

bool display_service_should_refresh(
    const sensor_sample_t *last_rendered,
    const sensor_sample_t *current,
    uint64_t now_ms,
    uint64_t last_refresh_ms)
{
    if (current == NULL || !current->valid) {
        return false;
    }

    if (last_rendered == NULL || !last_rendered->valid) {
        return true;
    }

    if (now_ms - last_refresh_ms < (uint64_t)CONFIG_APP_DISPLAY_MIN_REFRESH_SEC * 1000ULL) {
        return false;
    }

    const float voc_delta = fabsf(current->voc_index - last_rendered->voc_index);
    const float pressure_delta = fabsf(current->pressure_pa - last_rendered->pressure_pa);
    const float temp_delta_centi = fabsf((current->temperature_c - last_rendered->temperature_c) * 100.0f);

    if (voc_delta >= ((float)CONFIG_APP_REFRESH_VOC_DELTA_X100 / 100.0f)) {
        return true;
    }
    if (pressure_delta >= (float)CONFIG_APP_REFRESH_PRESSURE_DELTA_PA) {
        return true;
    }
    if (temp_delta_centi >= (float)CONFIG_APP_REFRESH_TEMP_DELTA_CENTI_C) {
        return true;
    }

    return false;
}

esp_err_t display_service_render(const sensor_sample_t *sample)
{
    if (sample == NULL || !sample->valid) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(display_service_init(), TAG, "display_init_failed");

    char line1[32] = {0};
    char line2[32] = {0};
    char line3[32] = {0};
    char line4[32] = {0};
    static const char *title = "SouthernIoT RnD";
    const int title_scale = 2;
    const int data_scale = 2;
    const int title_h = 7 * title_scale;
    const int data_h = 7 * data_scale;
    const int title_y = 4;
    const int divider_y = title_y + title_h + 2;
    const int line_y0 = divider_y + 6;
    const int line_step = data_h + 4;
    int title_x = (s_width - framebuffer_text_width_scaled(title, title_scale)) / 2;
    if (title_x < 0) {
        title_x = 0;
    }

    snprintf(line1, sizeof(line1), "VOC: %.1f", sample->voc_index);
    snprintf(line2, sizeof(line2), "PRESS: %.0f", sample->pressure_pa);
    snprintf(line3, sizeof(line3), "TEMP: %.2f", sample->temperature_c);
    snprintf(line4, sizeof(line4), "BATT: %.2f", sample->battery_v);

    framebuffer_clear_white();
    framebuffer_draw_text_scaled(title_x, title_y, title, PIXEL_BLACK, title_scale);
    framebuffer_fill_rect(0, divider_y, s_width, 2, PIXEL_RED);
    framebuffer_draw_text_scaled(6, line_y0 + (line_step * 0), line1, PIXEL_BLACK, data_scale);
    framebuffer_draw_text_scaled(6, line_y0 + (line_step * 1), line2, PIXEL_RED, data_scale);
    framebuffer_draw_text_scaled(6, line_y0 + (line_step * 2), line3, PIXEL_BLACK, data_scale);
    framebuffer_draw_text_scaled(6, line_y0 + (line_step * 3), line4, PIXEL_RED, data_scale);

    ESP_RETURN_ON_ERROR(display_flush(), TAG, "render_flush_failed");
    ESP_LOGI(
        TAG,
        "render_data voc=%.2f pressure=%.1f temp=%.2f batt=%.2f",
        sample->voc_index,
        sample->pressure_pa,
        sample->temperature_c,
        sample->battery_v);
    return ESP_OK;
}
