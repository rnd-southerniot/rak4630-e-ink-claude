#include "display_epd_ssd1680.h"

#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "DISPLAY";

#define EPD_CMD_SW_RESET 0x12
#define EPD_CMD_DRIVER_OUTPUT_CONTROL 0x01
#define EPD_CMD_DATA_ENTRY_MODE 0x11
#define EPD_CMD_SET_RAM_X 0x44
#define EPD_CMD_SET_RAM_Y 0x45
#define EPD_CMD_SET_RAM_X_COUNTER 0x4E
#define EPD_CMD_SET_RAM_Y_COUNTER 0x4F
#define EPD_CMD_WRITE_RAM_BW 0x24
#define EPD_CMD_WRITE_RAM_RED 0x26
#define EPD_CMD_UPDATE_CONTROL_2 0x22
#define EPD_CMD_MASTER_ACTIVATE 0x20
#define EPD_CMD_SET_BORDER 0x3C
#define EPD_CMD_TEMP_SENSOR 0x18
#define EPD_CMD_WRITE_VCOM 0x2C
#define EPD_CMD_GATE_VOLTAGE 0x03
#define EPD_CMD_SOURCE_VOLTAGE 0x04

typedef struct {
    bool initialized;
    display_epd_ssd1680_config_t cfg;
    spi_device_handle_t spi;
    bool reset_busy_pulse_seen;
    bool refresh_busy_pulse_seen;
    uint32_t spi_tx_calls;
    uint32_t spi_tx_bytes;
} epd_state_t;

static epd_state_t s_epd;

static bool epd_busy_active(void)
{
    const int level = gpio_get_level(s_epd.cfg.pin_busy);
    if (s_epd.cfg.busy_active_high) {
        return level == 1;
    }
    return level == 0;
}

static int epd_row_bytes(void)
{
    if (s_epd.cfg.width <= 0) {
        return 0;
    }
    return (s_epd.cfg.width + 7) / 8;
}

static size_t epd_frame_bytes(void)
{
    if (s_epd.cfg.width <= 0 || s_epd.cfg.height <= 0) {
        return 0;
    }
    return (size_t)epd_row_bytes() * (size_t)s_epd.cfg.height;
}

static esp_err_t epd_spi_tx(const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    const size_t chunk = 2048;
    size_t offset = 0;
    while (offset < len) {
        const size_t part = (len - offset > chunk) ? chunk : (len - offset);
        spi_transaction_t t = {0};
        t.length = part * 8;
        t.tx_buffer = data + offset;
        ESP_RETURN_ON_ERROR(spi_device_transmit(s_epd.spi, &t), TAG, "spi_tx_failed");
        offset += part;
        s_epd.spi_tx_calls++;
        s_epd.spi_tx_bytes += (uint32_t)part;
    }
    return ESP_OK;
}

static esp_err_t epd_send_cmd(uint8_t cmd)
{
    if (s_epd.cfg.pin_dc < 0) {
        return ESP_ERR_INVALID_STATE;
    }
    ESP_RETURN_ON_ERROR(gpio_set_level(s_epd.cfg.pin_dc, 0), TAG, "dc_low_failed");
    return epd_spi_tx(&cmd, 1);
}

static esp_err_t epd_send_data(const uint8_t *data, size_t len)
{
    if (s_epd.cfg.pin_dc < 0) {
        return ESP_ERR_INVALID_STATE;
    }
    ESP_RETURN_ON_ERROR(gpio_set_level(s_epd.cfg.pin_dc, 1), TAG, "dc_high_failed");
    return epd_spi_tx(data, len);
}

static esp_err_t epd_wait_idle(uint32_t timeout_ms, bool *saw_busy_active)
{
    const TickType_t step = pdMS_TO_TICKS(5);
    const uint32_t wait_assert_ms = 80;
    uint32_t waited_ms = 0;
    bool active = false;

    // BUSY can assert a few ms after a command is issued; watch a short window first.
    while (waited_ms < wait_assert_ms) {
        if (epd_busy_active()) {
            active = true;
            break;
        }
        vTaskDelay(step);
        waited_ms += 5;
    }

    while (active && epd_busy_active()) {
        if (waited_ms >= timeout_ms) {
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(step);
        waited_ms += 5;
    }

    if (saw_busy_active != NULL) {
        *saw_busy_active = active;
    }
    return ESP_OK;
}

static void epd_reset(void)
{
    if (s_epd.cfg.pin_rst < 0) {
        return;
    }
    (void)gpio_set_level(s_epd.cfg.pin_rst, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    (void)gpio_set_level(s_epd.cfg.pin_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

static esp_err_t epd_set_window(void)
{
    const uint8_t x_start = (uint8_t)s_epd.cfg.xram_offset;
    const uint8_t x_end = (uint8_t)(x_start + epd_row_bytes() - 1);
    const uint16_t y_end = (uint16_t)(s_epd.cfg.height - 1);

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
    if (cfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (cfg->width <= 0 || cfg->height <= 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_epd.initialized) {
        return ESP_OK;
    }
    if (cfg->pin_cs < 0 || cfg->pin_dc < 0 || cfg->pin_busy < 0 || cfg->pin_pwr < 0) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(&s_epd, 0, sizeof(s_epd));
    s_epd.cfg = *cfg;

    uint64_t out_mask = (1ULL << cfg->pin_dc);
    if (cfg->pin_rst >= 0) {
        out_mask |= (1ULL << cfg->pin_rst);
    }
    if (!cfg->pwr_input_pullup) {
        out_mask |= (1ULL << cfg->pin_pwr);
    }

    const gpio_config_t io_out = {
        .pin_bit_mask = out_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    if (out_mask != 0) {
        ESP_RETURN_ON_ERROR(gpio_config(&io_out), TAG, "gpio_out_cfg_failed");
    }

    const gpio_config_t io_busy = {
        .pin_bit_mask = (1ULL << cfg->pin_busy),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io_busy), TAG, "gpio_busy_cfg_failed");

    if (cfg->pwr_input_pullup) {
        const gpio_config_t io_pwr_input = {
            .pin_bit_mask = (1ULL << cfg->pin_pwr),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = cfg->pwr_active_high ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
            .pull_down_en = cfg->pwr_active_high ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&io_pwr_input), TAG, "gpio_pwr_input_cfg_failed");
    } else {
        ESP_RETURN_ON_ERROR(gpio_set_level(cfg->pin_pwr, cfg->pwr_active_high ? 1 : 0), TAG, "pwr_set_failed");
    }
    ESP_RETURN_ON_ERROR(gpio_set_level(cfg->pin_dc, 1), TAG, "dc_set_failed");
    if (cfg->pin_rst >= 0) {
        ESP_RETURN_ON_ERROR(gpio_set_level(cfg->pin_rst, 1), TAG, "rst_set_failed");
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(
        TAG,
        "busy_cfg pin=%d active_high=%d level=%d pwr_active_high=%d pwr_input_pullup=%d pwr_level=%d",
        cfg->pin_busy,
        cfg->busy_active_high,
        gpio_get_level(cfg->pin_busy),
        cfg->pwr_active_high,
        cfg->pwr_input_pullup,
        gpio_get_level(cfg->pin_pwr));

    const spi_bus_config_t bus_cfg = {
        .mosi_io_num = cfg->pin_mosi,
        .miso_io_num = cfg->pin_miso,
        .sclk_io_num = cfg->pin_sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = (int)epd_frame_bytes(),
    };
    esp_err_t err = spi_bus_initialize(cfg->spi_host, &bus_cfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    const spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = cfg->spi_mhz * 1000 * 1000,
        .mode = 0,
        .spics_io_num = cfg->pin_cs,
        .queue_size = 1,
    };
    ESP_RETURN_ON_ERROR(spi_bus_add_device(cfg->spi_host, &dev_cfg, &s_epd.spi), TAG, "spi_add_device_failed");

    epd_reset();

    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_SW_RESET), TAG, "sw_reset_cmd_failed");
    bool saw_busy = false;
    ESP_RETURN_ON_ERROR(epd_wait_idle(3000, &saw_busy), TAG, "sw_reset_busy_timeout");
    s_epd.reset_busy_pulse_seen = saw_busy;
    ESP_LOGI(TAG, "probe_reset_busy_pulse seen=%d", saw_busy);

    const uint16_t h = (uint16_t)(cfg->height - 1);
    const uint8_t drv_out[3] = {(uint8_t)(h & 0xFF), (uint8_t)(h >> 8), 0x00};
    const uint8_t data_entry = 0x03;
    const uint8_t border = 0x05;
    const uint8_t vcom = 0x36;
    const uint8_t gate_voltage = 0x17;
    const uint8_t source_voltage[3] = {0x41, 0x00, 0x32};
    const uint8_t temp = 0x80;
    const uint8_t update_ctl[2] = {0x00, 0x80};

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
    if (!s_epd.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (bw_framebuffer == NULL || len != epd_frame_bytes()) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(epd_set_window(), TAG, "set_window_failed");
    ESP_RETURN_ON_ERROR(epd_set_cursor(), TAG, "set_cursor_failed");
    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_WRITE_RAM_BW), TAG, "write_bw_cmd_failed");
    ESP_RETURN_ON_ERROR(epd_send_data(bw_framebuffer, len), TAG, "write_bw_data_failed");

    // Tri-color SSD1680 RED plane: 0 means white, 1 means red.
    static const uint8_t red_plane_white[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
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

    const uint8_t update = s_epd.cfg.partial_updates ? 0xFC : 0xF4;
    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_UPDATE_CONTROL_2), TAG, "update_ctl2_cmd_failed");
    ESP_RETURN_ON_ERROR(epd_send_data(&update, 1), TAG, "update_ctl2_data_failed");
    ESP_RETURN_ON_ERROR(epd_send_cmd(EPD_CMD_MASTER_ACTIVATE), TAG, "master_activate_failed");
    bool saw_busy = false;
    /* Tri-color (BWR) SSD1680 full refresh takes ~15-20 s — 5 s was far too short and reported a
     * spurious refresh_busy_timeout while the panel was still updating. */
    ESP_RETURN_ON_ERROR(epd_wait_idle(20000, &saw_busy), TAG, "refresh_busy_timeout");
    s_epd.refresh_busy_pulse_seen = saw_busy;
    ESP_LOGI(TAG, "probe_refresh_busy_pulse seen=%d", saw_busy);
    if (!saw_busy) {
        // Some panel/module combinations may not expose BUSY reliably. Provide a conservative refresh delay fallback.
        vTaskDelay(pdMS_TO_TICKS(CONFIG_APP_DISPLAY_REFRESH_FALLBACK_MS));
    }
    return ESP_OK;
}

void display_epd_ssd1680_get_diag(display_epd_ssd1680_diag_t *diag)
{
    if (diag == NULL) {
        return;
    }

    if (!s_epd.initialized) {
        memset(diag, 0, sizeof(*diag));
        return;
    }

    diag->reset_busy_pulse_seen = s_epd.reset_busy_pulse_seen;
    diag->refresh_busy_pulse_seen = s_epd.refresh_busy_pulse_seen;
    diag->busy_level_snapshot = gpio_get_level(s_epd.cfg.pin_busy);
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
