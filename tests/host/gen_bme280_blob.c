/*
 * gen_bme280_blob.c — emit the BME280 I2C device-profile as a prov-profile NVS blob (v2).
 *
 * BME280 over I2C@0x76: 2 measurands (temp C, pressure hPa) in the order profile_reader fills them
 * (values[0]=temp, values[1]=pressure). Payload (7 B): [schema][0x10][flags][temp_i16 x100][press_u16 x10].
 *   cc -I ../../firmware/components/device_profile/include gen_bme280_blob.c \
 *      ../../firmware/components/device_profile/device_profile.c -lm -o /tmp/gb && /tmp/gb
 */
#include "device_profile.h"
#include "telemetry.h"

#include <stdio.h>

static const dp_measurand_t MEAS[] = {
    {0, 0, DP_F32, DP_ABCD, 1.0f, 0.0f}, /* 0 temperature_c (I2C: reg fields unused) */
    {0, 0, DP_F32, DP_ABCD, 1.0f, 0.0f}, /* 1 pressure_hpa */
};
static const dp_field_t FIELDS[] = {
    {0, 0, DP_ENC_I16, 100.0f}, /* temp x100  (-40..85 C) */
    {1, 2, DP_ENC_U16, 10.0f},  /* pressure x10 hPa (300..1100 hPa) */
};
static const device_profile_t BME280 = {
    .device_byte = TELEMETRY_DEV_BME280, /* 0x10 */
    .bus_kind = DP_BUS_I2C,
    .i2c_addr = 0x76,
    .sensor_type = DP_SENSOR_BME280,
    .n_meas = 2,
    .meas = MEAS,
    .total_len = 7, /* 3 header + 4 body */
    .n_fields = 2,
    .fields = FIELDS,
};

int main(void)
{
    uint8_t blob[256];
    const size_t n = dp_serialize(&BME280, blob, sizeof(blob));
    if (n == 0) {
        fprintf(stderr, "serialize failed\n");
        return 1;
    }
    printf("# BME280 profile: %zu B, device_byte=0x%02X, i2c@0x%02X, %u meas, %u B payload\n", n,
           BME280.device_byte, BME280.i2c_addr, BME280.n_meas, BME280.total_len);
    printf("prov-profile ");
    for (size_t i = 0; i < n; ++i) {
        printf("%02x", blob[i]);
    }
    printf("\n");
    return 0;
}
