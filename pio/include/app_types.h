#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float voc_index;
    float pressure_pa;
    float temperature_c;
    float battery_v;
    bool valid;
    uint64_t timestamp_ms;
} sensor_sample_t;
