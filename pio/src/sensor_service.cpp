/*
 * Sensor service — Arduino port for nRF52840 (RAK4630)
 * SGP40 VOC + BMP280 pressure/temperature via Wire I2C.
 * Battery voltage via nRF52840 internal ADC (VBAT pin).
 * All sensor I2C protocol logic preserved from ESP-IDF version.
 */

#include <Arduino.h>
#include <string.h>

#include "sensor_service.h"
#include "i2c_bus.h"
#include "sensirion_gas_index_algorithm.h"
#include "board_pins.h"

static const char *TAG = "SENSOR";

static bool s_sgp40_present;
static bool s_bmp280_present;

/* --- BMP280 calibration data (Bosch datasheet section 3.11.3) --- */
typedef struct {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
} bmp280_calib_t;

static bmp280_calib_t s_bmp280_calib;
static bool s_bmp280_calib_valid;
static int32_t s_bmp280_t_fine;

/* --- Sensirion VOC Gas Index Algorithm state --- */
static GasIndexAlgorithmParams s_voc_params;
static bool s_voc_algo_initialized;

#define SGP40_MEASURE_RAW_DELAY_MS 35U
#define SGP40_DEFAULT_RH_TICKS    0x8000U
#define SGP40_DEFAULT_T_TICKS     0x6666U

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

    esp_err_t err = i2c_bus_write((uint8_t)APP_SGP40_ADDR, cmd, sizeof(cmd));
    if (err != ESP_OK) return err;
    delay(1);

    err = i2c_bus_read((uint8_t)APP_SGP40_ADDR, rx, sizeof(rx));
    if (err != ESP_OK) {
        uint8_t rx_short[6] = {0};
        err = i2c_bus_read((uint8_t)APP_SGP40_ADDR, rx_short, sizeof(rx_short));
        if (err != ESP_OK) return err;

        for (size_t i = 0; i < 2; i++) {
            const uint8_t *chunk = &rx_short[i * 3];
            if (sensirion_crc8(chunk, 2) != chunk[2]) return ESP_ERR_INVALID_CRC;
            identity->sgp40_serial_words[i] = (uint16_t)((chunk[0] << 8) | chunk[1]);
        }
        identity->sgp40_serial_words[2] = 0;
        identity->sgp40_serial_ok = true;
        return ESP_OK;
    }

    for (size_t i = 0; i < 3; i++) {
        const uint8_t *chunk = &rx[i * 3];
        if (sensirion_crc8(chunk, 2) != chunk[2]) return ESP_ERR_INVALID_CRC;
        identity->sgp40_serial_words[i] = (uint16_t)((chunk[0] << 8) | chunk[1]);
    }
    identity->sgp40_serial_ok = true;
    return ESP_OK;
}

static float sgp40_raw_to_voc_index(uint16_t raw_signal)
{
    if (!s_voc_algo_initialized) return -1.0f;
    int32_t voc_index = 0;
    GasIndexAlgorithm_process(&s_voc_params, (int32_t)raw_signal, &voc_index);
    return (float)voc_index;
}

static esp_err_t sgp40_measure_raw_signal(uint16_t *raw_signal)
{
    if (raw_signal == NULL) return ESP_ERR_INVALID_ARG;

    uint8_t cmd[8] = {
        0x26, 0x0F,
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
        last_err = i2c_bus_write((uint8_t)APP_SGP40_ADDR, cmd, sizeof(cmd));
        if (last_err != ESP_OK) {
            ESP_LOGW(TAG, "sgp40_write_retry attempt=%d err=%s", attempt, esp_err_to_name(last_err));
            delay(10);
            continue;
        }

        delay(SGP40_MEASURE_RAW_DELAY_MS);
        uint8_t rx[3] = {0};
        last_err = i2c_bus_read((uint8_t)APP_SGP40_ADDR, rx, sizeof(rx));
        if (last_err != ESP_OK) {
            ESP_LOGW(TAG, "sgp40_read_retry attempt=%d err=%s", attempt, esp_err_to_name(last_err));
            delay(10);
            continue;
        }

        if (sensirion_crc8(rx, 2) != rx[2]) {
            last_err = ESP_ERR_INVALID_CRC;
            ESP_LOGW(TAG, "sgp40_crc_retry attempt=%d", attempt);
            delay(10);
            continue;
        }

        *raw_signal = (uint16_t)((rx[0] << 8) | rx[1]);
        return ESP_OK;
    }
    return last_err;
}

static esp_err_t bmp280_read_chip_id(uint8_t *chip_id)
{
    if (chip_id == NULL) return ESP_ERR_INVALID_ARG;
    const uint8_t reg = 0xD0;
    return i2c_bus_write_read((uint8_t)APP_BMP280_ADDR, &reg, 1, chip_id, 1);
}

static esp_err_t bmp280_read_calibration(void)
{
    const uint8_t reg = 0x88;
    uint8_t buf[26] = {0};
    esp_err_t err = i2c_bus_write_read((uint8_t)APP_BMP280_ADDR, &reg, 1, buf, sizeof(buf));
    if (err != ESP_OK) return err;

    s_bmp280_calib.dig_T1 = (uint16_t)((uint16_t)buf[0] | ((uint16_t)buf[1] << 8));
    s_bmp280_calib.dig_T2 = (int16_t)((uint16_t)buf[2] | ((uint16_t)buf[3] << 8));
    s_bmp280_calib.dig_T3 = (int16_t)((uint16_t)buf[4] | ((uint16_t)buf[5] << 8));
    s_bmp280_calib.dig_P1 = (uint16_t)((uint16_t)buf[6] | ((uint16_t)buf[7] << 8));
    s_bmp280_calib.dig_P2 = (int16_t)((uint16_t)buf[8] | ((uint16_t)buf[9] << 8));
    s_bmp280_calib.dig_P3 = (int16_t)((uint16_t)buf[10] | ((uint16_t)buf[11] << 8));
    s_bmp280_calib.dig_P4 = (int16_t)((uint16_t)buf[12] | ((uint16_t)buf[13] << 8));
    s_bmp280_calib.dig_P5 = (int16_t)((uint16_t)buf[14] | ((uint16_t)buf[15] << 8));
    s_bmp280_calib.dig_P6 = (int16_t)((uint16_t)buf[16] | ((uint16_t)buf[17] << 8));
    s_bmp280_calib.dig_P7 = (int16_t)((uint16_t)buf[18] | ((uint16_t)buf[19] << 8));
    s_bmp280_calib.dig_P8 = (int16_t)((uint16_t)buf[20] | ((uint16_t)buf[21] << 8));
    s_bmp280_calib.dig_P9 = (int16_t)((uint16_t)buf[22] | ((uint16_t)buf[23] << 8));

    s_bmp280_calib_valid = true;
    ESP_LOGI(TAG, "bmp280_calib_ok dig_T1=%u dig_T2=%d dig_T3=%d dig_P1=%u",
             s_bmp280_calib.dig_T1, s_bmp280_calib.dig_T2,
             s_bmp280_calib.dig_T3, s_bmp280_calib.dig_P1);
    return ESP_OK;
}

static esp_err_t bmp280_trigger_forced(void)
{
    const uint8_t cfg_cmd[2] = {0xF5, 0x00};
    esp_err_t err = i2c_bus_write((uint8_t)APP_BMP280_ADDR, cfg_cmd, sizeof(cfg_cmd));
    if (err != ESP_OK) return err;
    const uint8_t meas_cmd[2] = {0xF4, 0x25};
    return i2c_bus_write((uint8_t)APP_BMP280_ADDR, meas_cmd, sizeof(meas_cmd));
}

/* Bosch BMP280 datasheet section 4.2.3 -- 32-bit integer compensation */
static int32_t bmp280_compensate_temperature(int32_t adc_T)
{
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)s_bmp280_calib.dig_T1 << 1))) *
                    ((int32_t)s_bmp280_calib.dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)s_bmp280_calib.dig_T1)) *
                      ((adc_T >> 4) - ((int32_t)s_bmp280_calib.dig_T1))) >> 12) *
                    ((int32_t)s_bmp280_calib.dig_T3)) >> 14;
    s_bmp280_t_fine = var1 + var2;
    return (s_bmp280_t_fine * 5 + 128) >> 8;
}

static uint32_t bmp280_compensate_pressure(int32_t adc_P)
{
    int32_t var1 = (((int32_t)s_bmp280_t_fine) >> 1) - (int32_t)64000;
    int32_t var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)s_bmp280_calib.dig_P6);
    var2 = var2 + ((var1 * ((int32_t)s_bmp280_calib.dig_P5)) << 1);
    var2 = (var2 >> 2) + (((int32_t)s_bmp280_calib.dig_P4) << 16);
    var1 = (((s_bmp280_calib.dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) +
            ((((int32_t)s_bmp280_calib.dig_P2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int32_t)s_bmp280_calib.dig_P1)) >> 15);
    if (var1 == 0) return 0;
    uint32_t p = (((uint32_t)(((int32_t)1048576) - adc_P) - (uint32_t)(var2 >> 12))) * 3125;
    if (p < 0x80000000U) {
        p = (p << 1) / ((uint32_t)var1);
    } else {
        p = (p / (uint32_t)var1) * 2;
    }
    var1 = (((int32_t)s_bmp280_calib.dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(p >> 2)) * ((int32_t)s_bmp280_calib.dig_P8)) >> 13;
    p = (uint32_t)((int32_t)p + ((var1 + var2 + (int32_t)s_bmp280_calib.dig_P7) >> 4));
    return p;
}

esp_err_t sensor_service_init(void)
{
    bool found = false;
    ESP_RETURN_ON_ERROR(i2c_bus_probe((uint8_t)APP_SGP40_ADDR, &found), TAG, "sgp40_probe_error");
    s_sgp40_present = found;

    found = false;
    ESP_RETURN_ON_ERROR(i2c_bus_probe((uint8_t)APP_BMP280_ADDR, &found), TAG, "bmp280_probe_error");
    s_bmp280_present = found;

    ESP_LOGI(TAG, "init_done sgp40=%d bmp280=%d sgp40_addr=0x%02X bmp280_addr=0x%02X",
             s_sgp40_present, s_bmp280_present, APP_SGP40_ADDR, APP_BMP280_ADDR);

    if (!s_sgp40_present && !s_bmp280_present) {
        ESP_LOGW(TAG, "no_sensors_found sgp40=%d bmp280=%d", s_sgp40_present, s_bmp280_present);
    } else if (!s_sgp40_present || !s_bmp280_present) {
        ESP_LOGW(TAG, "partial_sensors sgp40=%d bmp280=%d", s_sgp40_present, s_bmp280_present);
    }

    if (s_bmp280_present) {
        esp_err_t err = bmp280_read_calibration();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "bmp280_calib_read_failed err=%s", esp_err_to_name(err));
        }
    }

    if (s_sgp40_present) {
        GasIndexAlgorithm_init(&s_voc_params, GasIndexAlgorithm_ALGORITHM_TYPE_VOC);
        s_voc_algo_initialized = true;
        ESP_LOGI(TAG, "voc_algorithm_init_ok type=sensirion_gas_index");
    }

#if APP_BATTERY_ADC_ENABLED
    /* nRF52840: enable VBAT measurement — done in read function */
    ESP_LOGI(TAG, "battery_adc_init_ok method=nrf52_vbat");
#endif

    return ESP_OK;
}

bool sensor_service_is_sgp40_present(void) { return s_sgp40_present; }
bool sensor_service_is_bmp280_present(void) { return s_bmp280_present; }

esp_err_t sensor_service_read_identity(sensor_identity_t *identity)
{
    if (identity == NULL) return ESP_ERR_INVALID_ARG;
    memset(identity, 0, sizeof(*identity));
    identity->sgp40_present = s_sgp40_present;
    identity->bmp280_present = s_bmp280_present;
    if (!s_sgp40_present || !s_bmp280_present) return ESP_ERR_NOT_FOUND;

    esp_err_t err = sgp40_read_serial(identity);
    if (err != ESP_OK) ESP_LOGW(TAG, "sgp40_serial_read_failed err=%s", esp_err_to_name(err));

    err = bmp280_read_chip_id(&identity->bmp280_chip_id);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "bmp280_chip_id_read_failed err=%s", esp_err_to_name(err));
        return err;
    }
    /* Accept BMP280 (0x58) and BME280 (0x60). The BME280 is register-compatible
     * for temperature/pressure (adds humidity, which this node does not use). */
    identity->bmp280_chip_id_ok = (identity->bmp280_chip_id == 0x58 ||
                                   identity->bmp280_chip_id == 0x60);

    ESP_LOGI(TAG, "identity sgp40_serial_ok=%d serial_words=%04X-%04X-%04X bmp280_chip_id=0x%02X bmp280_id_ok=%d",
             identity->sgp40_serial_ok,
             identity->sgp40_serial_words[0], identity->sgp40_serial_words[1], identity->sgp40_serial_words[2],
             identity->bmp280_chip_id, identity->bmp280_chip_id_ok);

    if (!identity->bmp280_chip_id_ok) return ESP_ERR_INVALID_RESPONSE;
    if (!identity->sgp40_serial_ok) return ESP_FAIL;
    return ESP_OK;
}

esp_err_t sensor_service_read_sgp40_identity(sensor_identity_t *identity)
{
    if (identity == NULL) return ESP_ERR_INVALID_ARG;
    memset(identity, 0, sizeof(*identity));
    identity->sgp40_present = s_sgp40_present;
    identity->bmp280_present = s_bmp280_present;
    if (!s_sgp40_present) return ESP_ERR_NOT_FOUND;

    esp_err_t err = sgp40_read_serial(identity);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "sgp40_serial_read_failed err=%s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "identity_sgp40 serial_ok=%d serial_words=%04X-%04X-%04X",
             identity->sgp40_serial_ok,
             identity->sgp40_serial_words[0], identity->sgp40_serial_words[1], identity->sgp40_serial_words[2]);
    return identity->sgp40_serial_ok ? ESP_OK : ESP_FAIL;
}

esp_err_t sensor_service_read_bmp280_identity(sensor_identity_t *identity)
{
    if (identity == NULL) return ESP_ERR_INVALID_ARG;
    memset(identity, 0, sizeof(*identity));
    identity->sgp40_present = s_sgp40_present;
    identity->bmp280_present = s_bmp280_present;
    if (!s_bmp280_present) return ESP_ERR_NOT_FOUND;

    esp_err_t err = bmp280_read_chip_id(&identity->bmp280_chip_id);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "bmp280_chip_id_read_failed err=%s", esp_err_to_name(err));
        return err;
    }
    /* Accept BMP280 (0x58) and BME280 (0x60). The BME280 is register-compatible
     * for temperature/pressure (adds humidity, which this node does not use). */
    identity->bmp280_chip_id_ok = (identity->bmp280_chip_id == 0x58 ||
                                   identity->bmp280_chip_id == 0x60);
    ESP_LOGI(TAG, "identity_bmp280 chip_id=0x%02X chip_id_ok=%d", identity->bmp280_chip_id, identity->bmp280_chip_id_ok);
    return identity->bmp280_chip_id_ok ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

esp_err_t sensor_service_read_basic_raw(uint32_t *pressure_raw, uint32_t *temperature_raw)
{
    if (pressure_raw == NULL || temperature_raw == NULL) return ESP_ERR_INVALID_ARG;
    if (!s_bmp280_present) return ESP_ERR_NOT_FOUND;

    esp_err_t err = bmp280_trigger_forced();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "bmp280_trigger_forced_fail err=%s", esp_err_to_name(err));
        return err;
    }
    delay(10);

    const uint8_t reg = 0xF7;
    uint8_t raw[6] = {0};
    err = i2c_bus_write_read((uint8_t)APP_BMP280_ADDR, &reg, 1, raw, sizeof(raw));
    if (err != ESP_OK) return err;

    *pressure_raw = ((uint32_t)raw[0] << 12) | ((uint32_t)raw[1] << 4) | ((uint32_t)raw[2] >> 4);
    *temperature_raw = ((uint32_t)raw[3] << 12) | ((uint32_t)raw[4] << 4) | ((uint32_t)raw[5] >> 4);
    return ESP_OK;
}

esp_err_t sensor_service_read_voc_only(float *voc_index)
{
    if (voc_index == NULL) return ESP_ERR_INVALID_ARG;
    if (!s_sgp40_present) return ESP_ERR_NOT_FOUND;

    uint16_t raw_signal = 0;
    ESP_RETURN_ON_ERROR(sgp40_measure_raw_signal(&raw_signal), TAG, "sgp40_measure_failed");
    *voc_index = sgp40_raw_to_voc_index(raw_signal);
    ESP_LOGI(TAG, "sgp40_data raw=%u voc_index=%.2f", (unsigned)raw_signal, (double)*voc_index);
    return ESP_OK;
}

esp_err_t sensor_service_read_bmp280(float *pressure_pa, float *temperature_c)
{
    if (pressure_pa == NULL || temperature_c == NULL) return ESP_ERR_INVALID_ARG;
    if (!s_bmp280_present || !s_bmp280_calib_valid) return ESP_ERR_NOT_FOUND;

    uint32_t pressure_raw = 0;
    uint32_t temperature_raw = 0;
    esp_err_t err = sensor_service_read_basic_raw(&pressure_raw, &temperature_raw);
    if (err != ESP_OK) return err;

    int32_t temp_comp = bmp280_compensate_temperature((int32_t)temperature_raw);
    uint32_t press_comp = bmp280_compensate_pressure((int32_t)pressure_raw);

    *temperature_c = (float)temp_comp / 100.0f;
    *pressure_pa = (float)press_comp;
    ESP_LOGI(TAG, "bmp280_data pressure=%.1fPa temperature=%.2fC", (double)*pressure_pa, (double)*temperature_c);
    return ESP_OK;
}

float sensor_service_read_battery_v(void)
{
#if APP_BATTERY_ADC_ENABLED
    /*
     * nRF52840 battery voltage measurement:
     * Use analogRead on VBAT pin with internal reference.
     * The nRF52 BSP provides readVBAT() or we use direct analogRead.
     */
    analogReference(AR_INTERNAL_3_0);  /* 3.0V internal reference */
    analogReadResolution(12);           /* 12-bit resolution */

    uint32_t vbat_raw = 0;
    for (int i = 0; i < 8; i++) {
        vbat_raw += analogRead(PIN_BATTERY_VBAT);
    }
    vbat_raw /= 8;

    /* Convert: raw * 3.0V / 4096 * VBAT_DIVIDER_COMP */
    float battery_v = ((float)vbat_raw * 3.0f / 4096.0f) * VBAT_DIVIDER_COMP;
    ESP_LOGI(TAG, "battery_adc raw=%lu battery_v=%.3f", (unsigned long)vbat_raw, (double)battery_v);

    /* Restore default reference for other analog reads */
    analogReference(AR_DEFAULT);
    return battery_v;
#else
    return (float)APP_BATTERY_DEFAULT_MV / 1000.0f;
#endif
}

esp_err_t sensor_service_read(sensor_sample_t *sample)
{
    if (sample == NULL) return ESP_ERR_INVALID_ARG;
    if (!s_sgp40_present || !s_bmp280_present) return ESP_ERR_NOT_FOUND;

    float pressure_pa = 0.0f;
    float temperature_c = 0.0f;
    esp_err_t err = sensor_service_read_bmp280(&pressure_pa, &temperature_c);
    if (err != ESP_OK) return err;

    float voc = 0.0f;
    err = sensor_service_read_voc_only(&voc);
    if (err != ESP_OK) return err;

    float battery_v = sensor_service_read_battery_v();

    sample->voc_index = voc;
    sample->pressure_pa = pressure_pa;
    sample->temperature_c = temperature_c;
    sample->battery_v = battery_v;
    sample->timestamp_ms = (uint64_t)millis();
    sample->valid = (voc >= 1.0f && pressure_pa >= 80000.0f && pressure_pa <= 120000.0f &&
                     temperature_c >= -40.0f && temperature_c <= 85.0f);

    return sample->valid ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}
