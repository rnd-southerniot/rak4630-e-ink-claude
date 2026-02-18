#pragma once

#include <stddef.h>
#include <stdint.h>

#include "app_types.h"

#define APP_PAYLOAD_V1_LEN 12

size_t app_payload_encode_v1(const sensor_sample_t *sample, uint8_t *out, size_t out_len);
void app_payload_hex(const uint8_t *payload, size_t len, char *out, size_t out_len);
