/*
 * I2C bus driver — Arduino Wire implementation for nRF52840 (RAK4630)
 * Replaces ESP-IDF i2c_master driver. Same API surface as the original.
 */

#include <Arduino.h>
#include <Wire.h>

#include "i2c_bus.h"
#include "board_pins.h"

static const char *TAG = "I2C";
static bool s_initialized = false;

esp_err_t i2c_bus_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    Wire.begin();
    Wire.setClock(APP_I2C_FREQ_HZ);
    s_initialized = true;

    ESP_LOGI(TAG, "init_ok sda=%d scl=%d freq_hz=%d",
             PIN_I2C_SDA, PIN_I2C_SCL, APP_I2C_FREQ_HZ);
    return ESP_OK;
}

esp_err_t i2c_bus_probe(uint8_t addr, bool *found)
{
    if (found == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    *found = (err == 0);

    /* err == 0 means ACK, err == 2 means NACK (not found), both are normal */
    if (err == 0 || err == 2) {
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t i2c_bus_write(uint8_t addr, const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    Wire.beginTransmission(addr);
    size_t written = Wire.write(data, len);
    uint8_t err = Wire.endTransmission();

    if (err != 0 || written != len) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t i2c_bus_read(uint8_t addr, uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t received = Wire.requestFrom(addr, (uint8_t)len);
    if (received != len) {
        /* Drain any partial data */
        while (Wire.available()) {
            Wire.read();
        }
        return ESP_FAIL;
    }

    for (size_t i = 0; i < len; i++) {
        data[i] = (uint8_t)Wire.read();
    }
    return ESP_OK;
}

esp_err_t i2c_bus_write_read(uint8_t addr, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len)
{
    if (tx == NULL || tx_len == 0 || rx == NULL || rx_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Write phase — send without STOP (repeated start) */
    Wire.beginTransmission(addr);
    Wire.write(tx, tx_len);
    uint8_t err = Wire.endTransmission(false); /* false = repeated START */
    if (err != 0) {
        return ESP_FAIL;
    }

    /* Read phase */
    size_t received = Wire.requestFrom(addr, (uint8_t)rx_len);
    if (received != rx_len) {
        while (Wire.available()) {
            Wire.read();
        }
        return ESP_FAIL;
    }

    for (size_t i = 0; i < rx_len; i++) {
        rx[i] = (uint8_t)Wire.read();
    }
    return ESP_OK;
}

uint32_t i2c_bus_scan(i2c_scan_summary_t *summary)
{
    i2c_scan_summary_t local = {};

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
            if (addr == (uint8_t)APP_SGP40_ADDR) {
                local.sgp40_found = true;
            }
            if (addr == (uint8_t)APP_BMP280_ADDR) {
                local.bmp280_found = true;
            }
            ESP_LOGI(TAG, "scan_found addr=0x%02X", addr);
        }
    }

    if (summary != NULL) {
        *summary = local;
    }

    ESP_LOGI(TAG, "scan_done found=%lu sgp40=%d bmp280=%d",
             (unsigned long)local.found_count, local.sgp40_found, local.bmp280_found);
    return local.found_count;
}
