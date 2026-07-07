/* display_rows.c — compiled per-sensor_type row tables (see display_rows.h). */
#include "display_rows.h"

#include <string.h>

#include "device_profile.h" /* dp_sensor_t: DP_SENSOR_BME280 / _SGP40 / _SHTC3 / _NONE */

static disp_row_t mk(const char *label, const char *unit, float value, uint8_t decimals)
{
    disp_row_t r;
    memset(&r, 0, sizeof(r));
    strncpy(r.label, label, sizeof(r.label) - 1);
    strncpy(r.unit, unit, sizeof(r.unit) - 1);
    r.value = value;
    r.decimals = decimals;
    return r;
}

int display_rows_build(uint8_t sensor_type, const sensor_sample_t *s, disp_row_t out[], int max)
{
    if (out == NULL || s == NULL || max <= 0) {
        return 0;
    }
    int n = 0;
#define PUSH(row)                                                                                  \
    do {                                                                                           \
        if (n < max) {                                                                             \
            out[n++] = (row);                                                                      \
        }                                                                                          \
    } while (0)

    switch (sensor_type) {
    case DP_SENSOR_BME280: /* Bosch temp + pressure (humidity when a BME280 driver is added) */
        PUSH(mk("TEMP", "C", s->temperature_c, 2));
        PUSH(mk("PRESS", "hPa", s->pressure_pa / 100.0f, 0));
        break;
    case DP_SENSOR_SGP40: /* Sensirion VOC index */
        PUSH(mk("VOC", "", s->voc_index, 1));
        break;
    case DP_SENSOR_SHTC3: /* temp/humidity (humidity not in sensor_sample_t yet) */
        PUSH(mk("TEMP", "C", s->temperature_c, 2));
        break;
    default: /* DP_SENSOR_NONE / unknown: legacy set */
        PUSH(mk("VOC", "", s->voc_index, 1));
        PUSH(mk("PRESS", "hPa", s->pressure_pa / 100.0f, 0));
        PUSH(mk("TEMP", "C", s->temperature_c, 2));
        break;
    }
    PUSH(mk("BATT", "V", s->battery_v, 2)); /* battery row always last */

#undef PUSH
    return n;
}
