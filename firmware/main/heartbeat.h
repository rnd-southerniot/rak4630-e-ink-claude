#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t heartbeat_start(void);
uint32_t heartbeat_get_toggle_count(void);
bool heartbeat_is_running(void);
