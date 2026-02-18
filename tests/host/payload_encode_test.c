#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_payload.h"

static int test_payload_vector(void)
{
    const sensor_sample_t input = {
        .voc_index = 100.0f,
        .pressure_pa = 100900.0f,
        .temperature_c = 29.0f,
        .battery_v = 4.0f,
        .valid = true,
        .timestamp_ms = 0,
    };

    const uint8_t expected[APP_PAYLOAD_V1_LEN] = {
        0x01, 0x27, 0x10, 0x00, 0x01, 0x8A, 0x24, 0x0B, 0x54, 0x0F, 0xA0, 0x01,
    };

    uint8_t actual[APP_PAYLOAD_V1_LEN] = {0};
    size_t len = app_payload_encode_v1(&input, actual, sizeof(actual));

    if (len != APP_PAYLOAD_V1_LEN) {
        fprintf(stderr, "len mismatch: got=%zu expected=%d\n", len, APP_PAYLOAD_V1_LEN);
        return 1;
    }

    if (memcmp(actual, expected, APP_PAYLOAD_V1_LEN) != 0) {
        char hex[APP_PAYLOAD_V1_LEN * 3] = {0};
        app_payload_hex(actual, APP_PAYLOAD_V1_LEN, hex, sizeof(hex));
        fprintf(stderr, "payload mismatch: %s\n", hex);
        return 1;
    }

    return 0;
}

int main(void)
{
    int rc = test_payload_vector();
    if (rc != 0) {
        return rc;
    }

    printf("PASS payload_encode_v1 deterministic vector\n");
    return 0;
}
