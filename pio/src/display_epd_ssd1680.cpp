/*
 * SSD1680 E-Ink display driver — Arduino SPI implementation for nRF52840
 * Replaces ESP-IDF SPI driver. Same display logic and command sequence.
 */

#include <Arduino.h>
#include <SPI.h>
#include <string.h>

#include "display_epd_ssd1680.h"
#include "board_pins.h"

static const char *TAG = "DISPLAY";

#define EPD_CMD_SW_RESET           0x12
#define EPD_CMD_DRIVER_OUTPUT_CONTROL 0x01
#define EPD_CMD_DATA_ENTRY_MODE    0x11
#define EPD_CMD_SET_RAM_X          0x44
#define EPD_CMD_SET_RAM_Y          0x45
#define EPD_CMD_SET_RAM_X_COUNTER  0x4E
#define EPD_CMD_SET_RAM_Y_COUNTER  0x4F
#define EPD_CMD_WRITE_RAM_BW       0x24
#define EPD_CMD_WRITE_RAM_RED      0x26
#define EPD_CMD_UPDATE_CONTROL_2   0x22
#define EPD_CMD_MASTER_ACTIVATE    0x20
#define EPD_CMD_SET_BORDER         0x3C
#define EPD_CMD_TEMP_SENSOR        0x18
#define EPD_CMD_WRITE_VCOM         0x2C
#define EPD_CMD_GATE_VOLTAGE       0x03
#define EPD_CMD_SOURCE_VOLTAGE     0x04

typedef struct {
    bool initialized;
    display_epd_ssd1680_config_t cfg;
    bool reset_busy_pulse_seen;
    bool refresh_busy_pulse_seen;
    uint32_t spi_tx_calls;
    uint32_t spi_tx_bytes;
} epd_state_t;

static epd_state_t s_epd;

static bool epd_busy_active(void)
{
    int level = digitalRead(s_epd.cfg.pin_busy);
    return s_epd.cfg.busy_active_high ? (level == HIGH) : (level == LOW);
}

static int epd_row_bytes(void)
{
    if (s_epd.cfg.width <= 0) return 0;
    return (s_epd.cfg.width + 7) / 8;
}

static size_t epd_frame_bytes(void)
{
    if (s_epd.cfg.width <= 0 || s_epd.cfg.height <= 0) return 0;
    return (size_t)epd_row_bytes() * (size_t)s_epd.cfg.height;
}

static esp_err_t epd_spi_tx(const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) return ESP_ERR_INVALID_ARG;

    SPI.beginTransaction(SPISettings(s_epd.cfg.spi_mhz * 1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(s_epd.cfg.pin_cs, LOW);
    for (size_t i = 0; i < len; i++) {
        SPI.transfer(data[i]);
    }
    digitalWrite(s_epd.cfg.pin_cs, HIGH);
    SPI.endTransaction();

    s_epd.spi_tx_calls++;
    s_epd.spi_tx_bytes += (uint32_t)len;
    return ESP_OK;
}

static esp_err_t epd_send_cmd(uint8_t cmd)
{
    if (s_epd.cfg.pin_dc < 0) return ESP_ERR_INVALID_STATE;
    digitalWrite(s_epd.cfg.pin_dc, LOW);
    return epd_spi_tx(&cmd, 1);
}

static esp_err_t epd_send_data(const uint8_t *data, size_t len)
{
    if (s_epd.cfg.pin_dc < 0) return ESP_ERR_INVALID_STATE;
    digitalWrite(s_epd.cfg.pin_dc, HIGH);
    return epd_spi_tx(data, len);
}

static esp_err_t epd_wait_idle(uint32_t timeout_ms, bool *saw_busy_active)
{
    const uint32_t step_ms = 5;
    const uint32_t wait_assert_ms = 80;
    uint32_t waited_ms = 0;
    bool active = false;

    while (waited_ms < wait_assert_ms) {
        if (epd_busy_active()) {
            active = true;
            break;
        }
        delay(step_ms);
        waited_ms += step_ms;
    }

    while (active && epd_busy_active()) {
        if (waited_ms >= timeout_ms) return ESP_ERR_TIMEOUT;
        delay(step_ms);
        waited_ms += step_ms;
    }

    if (saw_busy_active != NULL) *saw_busy_active = active;
    return ESP_OK;
}

static void epd_reset(void)
{
    if (s_epd.cfg.pin_rst < 0) return;
    digitalWrite(s_epd.cfg.pin_rst, LOW);
    delay(10);
    digitalWrite(s_epd.cfg.pin_rst, HIGH);
    delay(10);
}

static esp_err_t epd_set_window(void)
{
    uint8_t x_start = (uint8_t)s_epd.cfg.xram_offset;
    uint8_t x_end = (uint8_t)(x_start + epd_row_bytes() - 1);
    uint16_t y_end = (uint16_t)(s_epd.cfg.height - 1);

    uint8_t x_range[2] = {x_start, x_end};
    uint8_t y_range[4] = {0x00, 0x00, (uint8_t)(y_end & 0xFF), (uint8_t)(y_end >> 8)};
    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_SET_RAM_X), TAG, "set_ram_x_cmd_failed");
    ESP_RETURN_ON_ERROR(epd_send_data(x_range, sizeof(x_range)), TAG, "set_ram_x_data_failed");
    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_SET_RAM_Y), TAG, "set_ram_y_cmd_failed");
    ESP_RETURN_ON_ERROR(epd_send_data(y_range, sizeof(y_range)), TAG, "set_ram_y_data_failed");
    return ESP_OK;
}

static esp_err_t epd_set_cursor(void)
{
    uint8_t x = (uint8_t)s_epd.cfg.xram_offset;
    uint8_t y[2] = {0x00, 0x00};

    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_SET_RAM_X_COUNTER), TAG, "set_ram_x_counter_failed");
    ESP_RETURN_ON_ERROR(epd_send_data(&x, 1), TAG, "set_ram_x_counter_data_failed");
    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_SET_RAM_Y_COUNTER), TAG, "set_ram_y_counter_failed");
    ESP_RETURN_ON_ERROR(epd_send_data(y, sizeof(y)), TAG, "set_ram_y_counter_data_failed");
    return ESP_OK;
}

esp_err_t display_epd_ssd1680_init(const display_epd_ssd1680_config_t *cfg)
{
    if (cfg == NULL) return ESP_ERR_INVALID_ARG;
    if (cfg->width <= 0 || cfg->height <= 0) return ESP_ERR_INVALID_ARG;
    if (s_epd.initialized) return ESP_OK;
    if (cfg->pin_cs < 0 || cfg->pin_dc < 0 || cfg->pin_busy < 0 || cfg->pin_pwr < 0) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(&s_epd, 0, sizeof(s_epd));
    s_epd.cfg = *cfg;

    /* Configure GPIO pins */
    pinMode(cfg->pin_dc, OUTPUT);
    pinMode(cfg->pin_cs, OUTPUT);
    digitalWrite(cfg->pin_cs, HIGH);

    if (cfg->pin_rst >= 0) {
        pinMode(cfg->pin_rst, OUTPUT);
        digitalWrite(cfg->pin_rst, HIGH);
    }

    /* BUSY pin — input with pull-up */
    pinMode(cfg->pin_busy, INPUT_PULLUP);

    /* Power enable pin */
    if (cfg->pwr_input_pullup) {
        pinMode(cfg->pin_pwr, INPUT_PULLUP);
    } else {
        pinMode(cfg->pin_pwr, OUTPUT);
        digitalWrite(cfg->pin_pwr, cfg->pwr_active_high ? HIGH : LOW);
    }

    digitalWrite(cfg->pin_dc, HIGH);
    delay(10);

    ESP_LOGI(TAG, "busy_cfg pin=%d active_high=%d level=%d pwr_active_high=%d pwr_input_pullup=%d pwr_level=%d",
             cfg->pin_busy, cfg->busy_active_high, digitalRead(cfg->pin_busy),
             cfg->pwr_active_high, cfg->pwr_input_pullup, digitalRead(cfg->pin_pwr));

    /* Initialize SPI */
    SPI.begin();

    /* Hardware reset */
    epd_reset();

    /* Software reset */
    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_SW_RESET), TAG, "sw_reset_cmd_failed");
    bool saw_busy = false;
    ESP_RETURN_ON_ERROR(epd_wait_idle(3000, &saw_busy), TAG, "sw_reset_busy_timeout");
    s_epd.reset_busy_pulse_seen = saw_busy;
    ESP_LOGI(TAG, "probe_reset_busy_pulse seen=%d", saw_busy);

    /* Panel configuration — same sequence as ESP-IDF version */
    uint16_t h = (uint16_t)(cfg->height - 1);
    uint8_t drv_out[3] = {(uint8_t)(h & 0xFF), (uint8_t)(h >> 8), 0x00};
    uint8_t data_entry = 0x03;
    uint8_t border = 0x05;
    uint8_t vcom = 0x36;
    uint8_t gate_voltage = 0x17;
    uint8_t source_voltage[3] = {0x41, 0x00, 0x32};
    uint8_t temp = 0x80;
    uint8_t update_ctl[2] = {0x00, 0x80};

    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_DATA_ENTRY_MODE), TAG, "data_entry_cmd_failed");
    ESP_RETURN_ON_ERROR(epd_send_data(&data_entry, 1), TAG, "data_entry_data_failed");
    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_SET_BORDER), TAG, "border_cmd_failed");
    ESP_RETURN_ON_ERROR(epd_send_data(&border, 1), TAG, "border_data_failed");
    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_WRITE_VCOM), TAG, "vcom_cmd_failed");
    ESP_RETURN_ON_ERROR(epd_send_data(&vcom, 1), TAG, "vcom_data_failed");
    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_GATE_VOLTAGE), TAG, "gate_voltage_cmd_failed");
    ESP_RETURN_ON_ERROR(epd_send_data(&gate_voltage, 1), TAG, "gate_voltage_data_failed");
    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_SOURCE_VOLTAGE), TAG, "source_voltage_cmd_failed");
    ESP_RETURN_ON_ERROR(epd_send_data(source_voltage, sizeof(source_voltage)), TAG, "source_voltage_data_failed");
    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_DRIVER_OUTPUT_CONTROL), TAG, "drv_out_cmd_failed");
    ESP_RETURN_ON_ERROR(epd_send_data(drv_out, sizeof(drv_out)), TAG, "drv_out_data_failed");
    ESP_RETURN_ON_ERROR(epd_set_window(), TAG, "set_window_failed");
    ESP_RETURN_ON_ERROR(epd_set_cursor(), TAG, "set_cursor_failed");
    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_TEMP_SENSOR), TAG, "temp_cmd_failed");
    ESP_RETURN_ON_ERROR(epd_send_data(&temp, 1), TAG, "temp_data_failed");
    ESP_RETURN_ON_ERROR(epd_send_cmd(0x21), TAG, "update_ctrl_cmd_failed");
    ESP_RETURN_ON_ERROR(epd_send_data(update_ctl, sizeof(update_ctl)), TAG, "update_ctrl_data_failed");

    s_epd.initialized = true;
    return ESP_OK;
}

esp_err_t display_epd_ssd1680_draw_frame(const uint8_t *framebuffer, size_t len)
{
    return display_epd_ssd1680_draw_frame_dual(framebuffer, NULL, len);
}

esp_err_t display_epd_ssd1680_draw_frame_dual(const uint8_t *bw_framebuffer, const uint8_t *red_framebuffer, size_t len)
{
    if (!s_epd.initialized) return ESP_ERR_INVALID_STATE;
    if (bw_framebuffer == NULL || len != epd_frame_bytes()) return ESP_ERR_INVALID_ARG;

    ESP_RETURN_ON_ERROR(epd_set_window(), TAG, "set_window_failed");
    ESP_RETURN_ON_ERROR(epd_set_cursor(), TAG, "set_cursor_failed");
    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_WRITE_RAM_BW), TAG, "write_bw_cmd_failed");
    ESP_RETURN_ON_ERROR(epd_send_data(bw_framebuffer, len), TAG, "write_bw_data_failed");

    static const uint8_t red_plane_white[32] = {0};
    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_WRITE_RAM_RED), TAG, "write_red_cmd_failed");
    if (red_framebuffer != NULL) {
        ESP_RETURN_ON_ERROR(epd_send_data(red_framebuffer, len), TAG, "write_red_data_failed");
    } else {
        size_t remaining = len;
        while (remaining > 0) {
            size_t chunk = remaining > sizeof(red_plane_white) ? sizeof(red_plane_white) : remaining;
            ESP_RETURN_ON_ERROR(epd_send_data(red_plane_white, chunk), TAG, "write_red_data_failed");
            remaining -= chunk;
        }
    }

    uint8_t update = s_epd.cfg.partial_updates ? 0xFC : 0xF4;
    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_UPDATE_CONTROL_2), TAG, "update_ctl2_cmd_failed");
    ESP_RETURN_ON_ERROR(epd_send_data(&update, 1), TAG, "update_ctl2_data_failed");
    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_MASTER_ACTIVATE), TAG, "master_activate_failed");
    bool saw_busy = false;
    ESP_RETURN_ON_ERROR(epd_wait_idle(5000, &saw_busy), TAG, "refresh_busy_timeout");
    s_epd.refresh_busy_pulse_seen = saw_busy;
    ESP_LOGI(TAG, "probe_refresh_busy_pulse seen=%d", saw_busy);
    if (!saw_busy) {
        delay(APP_DISPLAY_REFRESH_FALLBACK_MS);
    }
    return ESP_OK;
}

void display_epd_ssd1680_get_diag(display_epd_ssd1680_diag_t *diag)
{
    if (diag == NULL) return;
    if (!s_epd.initialized) {
        memset(diag, 0, sizeof(*diag));
        return;
    }

    diag->reset_busy_pulse_seen = s_epd.reset_busy_pulse_seen;
    diag->refresh_busy_pulse_seen = s_epd.refresh_busy_pulse_seen;
    diag->busy_level_snapshot = digitalRead(s_epd.cfg.pin_busy);
    diag->spi_tx_calls = s_epd.spi_tx_calls;
    diag->spi_tx_bytes = s_epd.spi_tx_bytes;
}

int display_epd_ssd1680_width(void)
{
    return s_epd.cfg.width;
}

int display_epd_ssd1680_height(void)
{
    return s_epd.cfg.height;
}
