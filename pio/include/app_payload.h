#pragma once

#include <stddef.h>
#include <stdint.h>

#include "app_types.h"

#define APP_PAYLOAD_V1_LEN 12
#define APP_PAYLOAD_V2_LEN 14  /* v1 + humidity_rh x100 (uint16) before status */

#ifdef __cplusplus
extern "C" {
#endif

size_t app_payload_encode_v1(const sensor_sample_t *sample, uint8_t *out, size_t out_len);
/* v2 is v1 plus 2 humidity bytes; decoders branch on the schema byte (out[0]). */
size_t app_payload_encode_v2(const sensor_sample_t *sample, uint8_t *out, size_t out_len);
void app_payload_hex(const uint8_t *payload, size_t len, char *out, size_t out_len);

#ifdef __cplusplus
}
#endif
