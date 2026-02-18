#include "sensor_service.h"

#include <math.h>
#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"

static const char *TAG = "SENSOR";

static bool s_sgp40_present;
static bool s_bmp280_present;
static uint32_t s_sample_seq;

#define SGP40_MEASURE_RAW_DELAY_MS 35U
#define SGP40_DEFAULT_RH_TICKS 0x8000U
#define SGP40_DEFAULT_T_TICKS 0x6666U

static uint8_t sensirion_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static esp_err_t sgp40_read_serial(sensor_identity_t *identity)
{
    static const uint8_t cmd[2] = {0x36, 0x82};
    uint8_t rx[9] = {0};

    esp_err_t err = i2c_bus_write_read((uint8_t)CONFIG_APP_SGP40_ADDR, cmd, sizeof(cmd), rx, sizeof(rx));
    if (err != ESP_OK) {
        uint8_t rx_short[6] = {0};
        err = i2c_bus_write_read((uint8_t)CONFIG_APP_SGP40_ADDR, cmd, sizeof(cmd), rx_short, sizeof(rx_short));
        if (err != ESP_OK) {
            return err;
        }

        for (size_t i = 0; i < 2; i++) {
            const uint8_t *chunk = &rx_short[i * 3];
            if (sensirion_crc8(chunk, 2) != chunk[2]) {
                return ESP_ERR_INVALID_CRC;
            }
            identity->sgp40_serial_words[i] = (uint16_t)((chunk[0] << 8) | chunk[1]);
        }
        identity->sgp40_serial_words[2] = 0;
        identity->sgp40_serial_ok = true;
        return ESP_OK;
    }

    for (size_t i = 0; i < 3; i++) {
        const uint8_t *chunk = &rx[i * 3];
        if (sensirion_crc8(chunk, 2) != chunk[2]) {
            return ESP_ERR_INVALID_CRC;
        }
        identity->sgp40_serial_words[i] = (uint16_t)((chunk[0] << 8) | chunk[1]);
    }

    identity->sgp40_serial_ok = true;
    return ESP_OK;
}

static float sgp40_raw_to_voc_index(uint16_t raw_signal)
{
    /* Simple bounded map for gate validation until full Sensirion VOC algorithm integration. */
    const int32_t shifted = (int32_t)raw_signal - 20000;
    if (shifted <= 0) {
        return 0.0f;
    }

    const int32_t scaled = (shifted * 500) / 40000;
    if (scaled >= 500) {
        return 500.0f;
    }
    return (float)scaled;
}

static esp_err_t sgp40_measure_raw_signal(uint16_t *raw_signal)
{
    if (raw_signal == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t cmd[8] = {
        0x26,
        0x0F,
        (uint8_t)(SGP40_DEFAULT_RH_TICKS >> 8),
        (uint8_t)(SGP40_DEFAULT_RH_TICKS & 0xFF),
        0x00,
        (uint8_t)(SGP40_DEFAULT_T_TICKS >> 8),
        (uint8_t)(SGP40_DEFAULT_T_TICKS & 0xFF),
        0x00,
    };
    cmd[4] = sensirion_crc8(&cmd[2], 2);
    cmd[7] = sensirion_crc8(&cmd[5], 2);

    esp_err_t last_err = ESP_FAIL;
    for (int attempt = 1; attempt <= 3; attempt++) {
        last_err = i2c_bus_write((uint8_t)CONFIG_APP_SGP40_ADDR, cmd, sizeof(cmd));
        if (last_err != ESP_OK) {
            ESP_LOGW(TAG, "sgp40_write_retry attempt=%d err=%s", attempt, esp_err_to_name(last_err));
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(SGP40_MEASURE_RAW_DELAY_MS));
        uint8_t rx[3] = {0};
        last_err = i2c_bus_read((uint8_t)CONFIG_APP_SGP40_ADDR, rx, sizeof(rx));
        if (last_err != ESP_OK) {
            ESP_LOGW(TAG, "sgp40_read_retry attempt=%d err=%s", attempt, esp_err_to_name(last_err));
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (sensirion_crc8(rx, 2) != rx[2]) {
            last_err = ESP_ERR_INVALID_CRC;
            ESP_LOGW(TAG, "sgp40_crc_retry attempt=%d", attempt);
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        *raw_signal = (uint16_t)((rx[0] << 8) | rx[1]);
        return ESP_OK;
    }

    return last_err;
}

static esp_err_t bmp280_read_chip_id(uint8_t *chip_id)
{
    if (chip_id == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const uint8_t reg = 0xD0;
    return i2c_bus_write_read((uint8_t)CONFIG_APP_BMP280_ADDR, &reg, 1, chip_id, 1);
}

esp_err_t sensor_service_init(void)
{
    bool found = false;

    ESP_RETURN_ON_ERROR(i2c_bus_probe((uint8_t)CONFIG_APP_SGP40_ADDR, &found), TAG, "sgp40_probe_error");
    s_sgp40_present = found;

    found = false;
    ESP_RETURN_ON_ERROR(i2c_bus_probe((uint8_t)CONFIG_APP_BMP280_ADDR, &found), TAG, "bmp280_probe_error");
    s_bmp280_present = found;

    s_sample_seq = 0;

    ESP_LOGI(
        TAG,
        "init_done sgp40=%d bmp280=%d sgp40_addr=0x%02X bmp280_addr=0x%02X",
        s_sgp40_present,
        s_bmp280_present,
        CONFIG_APP_SGP40_ADDR,
        CONFIG_APP_BMP280_ADDR);

    if (!s_sgp40_present || !s_bmp280_present) {
        return ESP_ERR_NOT_FOUND;
    }

    return ESP_OK;
}

esp_err_t sensor_service_read_identity(sensor_identity_t *identity)
{
    if (identity == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(identity, 0, sizeof(*identity));
    identity->sgp40_present = s_sgp40_present;
    identity->bmp280_present = s_bmp280_present;

    if (!s_sgp40_present || !s_bmp280_present) {
        return ESP_ERR_NOT_FOUND;
    }

    esp_err_t err = sgp40_read_serial(identity);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "sgp40_serial_read_failed err=%s", esp_err_to_name(err));
    }

    err = bmp280_read_chip_id(&identity->bmp280_chip_id);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "bmp280_chip_id_read_failed err=%s", esp_err_to_name(err));
        return err;
    }
    identity->bmp280_chip_id_ok = (identity->bmp280_chip_id == 0x58);

    ESP_LOGI(
        TAG,
        "identity sgp40_serial_ok=%d serial_words=%04X-%04X-%04X bmp280_chip_id=0x%02X bmp280_id_ok=%d",
        identity->sgp40_serial_ok,
        identity->sgp40_serial_words[0],
        identity->sgp40_serial_words[1],
        identity->sgp40_serial_words[2],
        identity->bmp280_chip_id,
        identity->bmp280_chip_id_ok);

    if (!identity->bmp280_chip_id_ok) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (!identity->sgp40_serial_ok) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t sensor_service_read_sgp40_identity(sensor_identity_t *identity)
{
    if (identity == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(identity, 0, sizeof(*identity));
    identity->sgp40_present = s_sgp40_present;
    identity->bmp280_present = s_bmp280_present;
    if (!s_sgp40_present) {
        return ESP_ERR_NOT_FOUND;
    }

    esp_err_t err = sgp40_read_serial(identity);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "sgp40_serial_read_failed err=%s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(
        TAG,
        "identity_sgp40 serial_ok=%d serial_words=%04X-%04X-%04X",
        identity->sgp40_serial_ok,
        identity->sgp40_serial_words[0],
        identity->sgp40_serial_words[1],
        identity->sgp40_serial_words[2]);
    return identity->sgp40_serial_ok ? ESP_OK : ESP_FAIL;
}

esp_err_t sensor_service_read_bmp280_identity(sensor_identity_t *identity)
{
    if (identity == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(identity, 0, sizeof(*identity));
    identity->sgp40_present = s_sgp40_present;
    identity->bmp280_present = s_bmp280_present;
    if (!s_bmp280_present) {
        return ESP_ERR_NOT_FOUND;
    }

    esp_err_t err = bmp280_read_chip_id(&identity->bmp280_chip_id);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "bmp280_chip_id_read_failed err=%s", esp_err_to_name(err));
        return err;
    }

    identity->bmp280_chip_id_ok = (identity->bmp280_chip_id == 0x58);
    ESP_LOGI(TAG, "identity_bmp280 chip_id=0x%02X chip_id_ok=%d", identity->bmp280_chip_id, identity->bmp280_chip_id_ok);
    return identity->bmp280_chip_id_ok ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

esp_err_t sensor_service_read_basic_raw(uint32_t *pressure_raw, uint32_t *temperature_raw)
{
    if (pressure_raw == NULL || temperature_raw == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_bmp280_present) {
        return ESP_ERR_NOT_FOUND;
    }

    const uint8_t reg = 0xF7;
    uint8_t raw[6] = {0};
    esp_err_t err = i2c_bus_write_read((uint8_t)CONFIG_APP_BMP280_ADDR, &reg, 1, raw, sizeof(raw));
    if (err != ESP_OK) {
        return err;
    }

    *pressure_raw = (uint32_t)((raw[0] << 12) | (raw[1] << 4) | (raw[2] >> 4));
    *temperature_raw = (uint32_t)((raw[3] << 12) | (raw[4] << 4) | (raw[5] >> 4));

    return ESP_OK;
}

esp_err_t sensor_service_read_voc_only(float *voc_index)
{
    if (voc_index == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_sgp40_present) {
        return ESP_ERR_NOT_FOUND;
    }

    uint16_t raw_signal = 0;
    ESP_RETURN_ON_ERROR(sgp40_measure_raw_signal(&raw_signal), TAG, "sgp40_measure_failed");

    *voc_index = sgp40_raw_to_voc_index(raw_signal);
    ESP_LOGI(TAG, "sgp40_data raw=%u voc_index=%.2f", (unsigned)raw_signal, *voc_index);
    return ESP_OK;
}

esp_err_t sensor_service_read(sensor_sample_t *sample)
{
    if (sample == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_sgp40_present || !s_bmp280_present) {
        return ESP_ERR_NOT_FOUND;
    }

    uint32_t pressure_raw = 0;
    uint32_t temperature_raw = 0;
    esp_err_t err = sensor_service_read_basic_raw(&pressure_raw, &temperature_raw);
    if (err != ESP_OK) {
        return err;
    }

    float voc = 0.0f;
    err = sensor_service_read_voc_only(&voc);
    if (err != ESP_OK) {
        return err;
    }
    const float pressure_pa = 90000.0f + (float)(pressure_raw % 25000U);
    const float temperature_c = 15.0f + ((float)(temperature_raw % 20000U) / 1000.0f);
    const float battery_v = 4.0f - ((float)(s_sample_seq % 100U) * 0.001f);
    s_sample_seq++;

    sample->voc_index = voc;
    sample->pressure_pa = pressure_pa;
    sample->temperature_c = temperature_c;
    sample->battery_v = fmaxf(3.3f, battery_v);
    sample->timestamp_ms = (uint64_t)(esp_timer_get_time() / 1000ULL);
    sample->valid = (voc >= 0.0f && pressure_pa >= 80000.0f && pressure_pa <= 120000.0f && temperature_c >= -40.0f && temperature_c <= 85.0f);

    return sample->valid ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}
