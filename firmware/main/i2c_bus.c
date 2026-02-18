#include "i2c_bus.h"

#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "I2C";

static i2c_port_t app_i2c_port(void)
{
    return (i2c_port_t)CONFIG_APP_I2C_PORT;
}

esp_err_t i2c_bus_init(void)
{
    const i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CONFIG_APP_I2C_SDA_GPIO,
        .scl_io_num = CONFIG_APP_I2C_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = CONFIG_APP_I2C_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_param_config(app_i2c_port(), &cfg));
    esp_err_t err = i2c_driver_install(app_i2c_port(), cfg.mode, 0, 0, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(
        TAG,
        "init_ok port=%d sda=%d scl=%d freq_hz=%d",
        CONFIG_APP_I2C_PORT,
        CONFIG_APP_I2C_SDA_GPIO,
        CONFIG_APP_I2C_SCL_GPIO,
        CONFIG_APP_I2C_FREQ_HZ);
    return ESP_OK;
}

esp_err_t i2c_bus_probe(uint8_t addr, bool *found)
{
    if (found == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        return ESP_ERR_NO_MEM;
    }

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    const esp_err_t err = i2c_master_cmd_begin(app_i2c_port(), cmd, pdMS_TO_TICKS(20));
    i2c_cmd_link_delete(cmd);
    *found = (err == ESP_OK);

    if (err == ESP_OK || err == ESP_ERR_TIMEOUT || err == ESP_FAIL) {
        return ESP_OK;
    }
    return err;
}

esp_err_t i2c_bus_write(uint8_t addr, const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_write_to_device(app_i2c_port(), addr, data, len, pdMS_TO_TICKS(50));
}

esp_err_t i2c_bus_read(uint8_t addr, uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_read_from_device(app_i2c_port(), addr, data, len, pdMS_TO_TICKS(100));
}

esp_err_t i2c_bus_write_read(uint8_t addr, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len)
{
    if (tx == NULL || tx_len == 0 || rx == NULL || rx_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_write_read_device(app_i2c_port(), addr, tx, tx_len, rx, rx_len, pdMS_TO_TICKS(100));
}

uint32_t i2c_bus_scan(i2c_scan_summary_t *summary)
{
    i2c_scan_summary_t local = {0};

    ESP_LOGI(TAG, "scan_start");
    for (uint8_t addr = 0x03; addr <= 0x77; addr++) {
        bool found = false;
        esp_err_t err = i2c_bus_probe(addr, &found);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "scan_probe_error addr=0x%02X err=%s", addr, esp_err_to_name(err));
            continue;
        }

        if (found) {
            local.found_count++;
            if (addr == (uint8_t)CONFIG_APP_SGP40_ADDR) {
                local.sgp40_found = true;
            }
            if (addr == (uint8_t)CONFIG_APP_BMP280_ADDR) {
                local.bmp280_found = true;
            }
            ESP_LOGI(TAG, "scan_found addr=0x%02X", addr);
        }
    }

    if (summary != NULL) {
        *summary = local;
    }

    ESP_LOGI(
        TAG,
        "scan_done found=%lu sgp40=%d bmp280=%d",
        (unsigned long)local.found_count,
        local.sgp40_found,
        local.bmp280_found);

    return local.found_count;
}
