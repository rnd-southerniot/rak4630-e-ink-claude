#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "compat.h"

typedef enum {
    LORAWAN_STATE_IDLE = 0,
    LORAWAN_STATE_JOINING,
    LORAWAN_STATE_JOINED,
} lorawan_state_t;

typedef struct {
    bool backend_active;
    uint32_t join_attempts;
    uint32_t join_successes;
    uint32_t join_failures;
    uint32_t retries;
    uint32_t uplink_ok;
    uint32_t uplink_fail;
    uint64_t last_join_ms;
} lorawan_metrics_t;

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t lorawan_service_init(void);
void lorawan_service_process(uint64_t now_ms);
lorawan_state_t lorawan_service_state(void);
bool lorawan_service_is_joined(void);
esp_err_t lorawan_service_set_backend_active(bool active);
void lorawan_service_get_metrics(lorawan_metrics_t *out);
esp_err_t lorawan_service_send(const uint8_t *payload, size_t len);

#ifdef __cplusplus
}
#endif
