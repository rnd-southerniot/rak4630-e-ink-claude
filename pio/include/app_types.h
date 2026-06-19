#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Which fields of a sensor_sample_t were actually measured (vs. defaulted). */
#define SENSOR_FIELD_VOC      (1u << 0)
#define SENSOR_FIELD_PRESSURE (1u << 1)
#define SENSOR_FIELD_TEMP     (1u << 2)
#define SENSOR_FIELD_HUMIDITY (1u << 3)
#define SENSOR_FIELD_BATTERY  (1u << 4)

/* New fields (humidity_rh, present_mask) are appended at the END so existing
 * C++ designated initializers that omit trailing members keep compiling on the
 * Arduino GCC (which rejects gapped designated initializers). */
typedef struct {
    float voc_index;
    float pressure_pa;
    float temperature_c;
    float battery_v;
    bool valid;
    uint64_t timestamp_ms;
    float humidity_rh;     /* relative humidity %, 0 when no humidity sensor present */
    uint32_t present_mask; /* bitwise-OR of SENSOR_FIELD_* that are populated */
} sensor_sample_t;
