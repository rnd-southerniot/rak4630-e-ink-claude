#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "compat.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t heartbeat_start(void);
void heartbeat_update(void);
uint32_t heartbeat_get_toggle_count(void);
bool heartbeat_is_running(void);

#ifdef __cplusplus
}
#endif
