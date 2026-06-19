/*
 * SHTC3 (RAK1901) temperature + humidity driver — Sensirion I2C, address 0x70.
 * Implements the sensor_driver_t interface. Contributes humidity (and temperature
 * only when no pressure/temperature sensor has already populated it).
 *
 * Protocol: wake (0x3517) -> measure normal-mode T-first, clock-stretch off
 * (0x7866) -> wait ~12 ms -> read 6 bytes [T_hi T_lo T_crc RH_hi RH_lo RH_crc]
 * -> sleep (0xB098). CRC8 poly 0x31, init 0xFF (Sensirion).
 */
#include <Arduino.h>
#include <string.h>

#include "sensor_driver.h"
#include "i2c_bus.h"

#ifndef APP_SHTC3_ADDR
#define APP_SHTC3_ADDR 0x70
#endif

static const char *TAG = "SENSOR";

static uint8_t shtc3_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static esp_err_t shtc3_cmd(uint16_t cmd)
{
    uint8_t buf[2] = {(uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF)};
    return i2c_bus_write((uint8_t)APP_SHTC3_ADDR, buf, sizeof(buf));
}

static esp_err_t shtc3_init(void)
{
    /* Wake, read ID, sleep. SHTC3 ID: (id & 0x083F) == 0x0807. */
    esp_err_t err = shtc3_cmd(0x3517);   /* wakeup */
    if (err != ESP_OK) return err;
    delay(1);
    (void)shtc3_cmd(0xB098);             /* sleep */
    ESP_LOGI(TAG, "shtc3_init_ok addr=0x%02X", APP_SHTC3_ADDR);
    return ESP_OK;
}

static esp_err_t shtc3_identity(char *buf, size_t buf_len, bool *ok)
{
    if (ok) *ok = false;
    esp_err_t err = shtc3_cmd(0x3517);   /* wakeup */
    if (err != ESP_OK) return err;
    delay(1);
    const uint8_t idcmd[2] = {0xEF, 0xC8};
    err = i2c_bus_write((uint8_t)APP_SHTC3_ADDR, idcmd, sizeof(idcmd));
    if (err != ESP_OK) { (void)shtc3_cmd(0xB098); return err; }
    delay(1);
    uint8_t rx[3] = {0};
    err = i2c_bus_read((uint8_t)APP_SHTC3_ADDR, rx, sizeof(rx));
    (void)shtc3_cmd(0xB098);             /* sleep */
    if (err != ESP_OK) return err;
    if (shtc3_crc8(rx, 2) != rx[2]) return ESP_ERR_INVALID_CRC;
    uint16_t id = (uint16_t)((rx[0] << 8) | rx[1]);
    bool valid = ((id & 0x083F) == 0x0807);
    if (ok) *ok = valid;
    if (buf && buf_len) snprintf(buf, buf_len, "id=0x%04X", id);
    ESP_LOGI(TAG, "shtc3_identity id=0x%04X ok=%d", id, valid);
    return valid ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

static esp_err_t shtc3_read(sensor_sample_t *sample)
{
    if (sample == NULL) return ESP_ERR_INVALID_ARG;

    esp_err_t err = shtc3_cmd(0x3517);   /* wakeup */
    if (err != ESP_OK) return err;
    delay(1);
    err = shtc3_cmd(0x7866);             /* measure: normal, T first, no stretch */
    if (err != ESP_OK) { (void)shtc3_cmd(0xB098); return err; }
    delay(15);

    uint8_t rx[6] = {0};
    err = i2c_bus_read((uint8_t)APP_SHTC3_ADDR, rx, sizeof(rx));
    (void)shtc3_cmd(0xB098);             /* sleep */
    if (err != ESP_OK) return err;
    if (shtc3_crc8(&rx[0], 2) != rx[2] || shtc3_crc8(&rx[3], 2) != rx[5]) {
        return ESP_ERR_INVALID_CRC;
    }

    uint16_t t_raw = (uint16_t)((rx[0] << 8) | rx[1]);
    uint16_t rh_raw = (uint16_t)((rx[3] << 8) | rx[4]);
    float temperature_c = -45.0f + 175.0f * ((float)t_raw / 65536.0f);
    float humidity_rh = 100.0f * ((float)rh_raw / 65536.0f);
    if (humidity_rh < 0.0f) humidity_rh = 0.0f;
    if (humidity_rh > 100.0f) humidity_rh = 100.0f;

    sample->humidity_rh = humidity_rh;
    sample->present_mask |= SENSOR_FIELD_HUMIDITY;
    /* Only supply temperature if no pressure/temp sensor already did. */
    if (!(sample->present_mask & SENSOR_FIELD_TEMP)) {
        sample->temperature_c = temperature_c;
        sample->present_mask |= SENSOR_FIELD_TEMP;
    }
    ESP_LOGI(TAG, "shtc3_data humidity=%.2f%% temperature=%.2fC", (double)humidity_rh, (double)temperature_c);
    return ESP_OK;
}

static sensor_driver_t s_shtc3 = {
    "SHTC3", (uint8_t)APP_SHTC3_ADDR, SENSOR_FIELD_HUMIDITY | SENSOR_FIELD_TEMP,
    shtc3_init, shtc3_identity, shtc3_read, false,
};

sensor_driver_t *shtc3_driver(void) { return &s_shtc3; }
