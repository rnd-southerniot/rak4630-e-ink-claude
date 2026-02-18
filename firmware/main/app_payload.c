#include "app_payload.h"

#include <math.h>
#include <stdio.h>

static uint16_t clamp_u16(int32_t v)
{
    if (v < 0) {
        return 0;
    }
    if (v > 65535) {
        return 65535;
    }
    return (uint16_t)v;
}

size_t app_payload_encode_v1(const sensor_sample_t *sample, uint8_t *out, size_t out_len)
{
    if (sample == NULL || out == NULL || out_len < APP_PAYLOAD_V1_LEN) {
        return 0;
    }

    // Payload schema v1:
    // [0] schema version
    // [1..2] voc_index x100 (uint16)
    // [3..6] pressure_pa (uint32)
    // [7..8] temperature_c x100 (int16)
    // [9..10] battery_mv (uint16)
    // [11] status flags
    const uint8_t schema = 1;
    const uint16_t voc_x100 = clamp_u16((int32_t)lroundf(sample->voc_index * 100.0f));
    const uint32_t pressure_pa = (sample->pressure_pa < 0.0f) ? 0U : (uint32_t)lroundf(sample->pressure_pa);
    const int16_t temp_centi = (int16_t)lroundf(sample->temperature_c * 100.0f);
    const uint16_t batt_mv = clamp_u16((int32_t)lroundf(sample->battery_v * 1000.0f));
    const uint8_t status_flags = sample->valid ? 0x01 : 0x00;

    out[0] = schema;
    out[1] = (uint8_t)(voc_x100 >> 8);
    out[2] = (uint8_t)(voc_x100 & 0xFF);
    out[3] = (uint8_t)(pressure_pa >> 24);
    out[4] = (uint8_t)((pressure_pa >> 16) & 0xFF);
    out[5] = (uint8_t)((pressure_pa >> 8) & 0xFF);
    out[6] = (uint8_t)(pressure_pa & 0xFF);
    out[7] = (uint8_t)((uint16_t)temp_centi >> 8);
    out[8] = (uint8_t)((uint16_t)temp_centi & 0xFF);
    out[9] = (uint8_t)(batt_mv >> 8);
    out[10] = (uint8_t)(batt_mv & 0xFF);
    out[11] = status_flags;

    return APP_PAYLOAD_V1_LEN;
}

void app_payload_hex(const uint8_t *payload, size_t len, char *out, size_t out_len)
{
    if (out == NULL || out_len == 0) {
        return;
    }

    if (payload == NULL || len == 0) {
        out[0] = '\0';
        return;
    }

    size_t used = 0;
    for (size_t i = 0; i < len; i++) {
        int written = snprintf(out + used, out_len - used, (i + 1 < len) ? "%02X " : "%02X", payload[i]);
        if (written <= 0 || (size_t)written >= (out_len - used)) {
            out[out_len - 1] = '\0';
            return;
        }
        used += (size_t)written;
    }
}
