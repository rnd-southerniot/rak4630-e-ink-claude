/*
 * display_rows.h — build the e-ink sensor rows from the active I2C sensor_type (dp_sensor_t).
 *
 * The row set (labels/units/decimals) is a compiled per-sensor_type table; values come from the
 * sensor_sample_t. This makes the display "dynamic per I2C profile" without adding text to the
 * device-profile blob: the provisioned profile's sensor_type (or, absent a profile, the detected
 * sensor) selects which rows are shown. A BATT row is always appended.
 */
#ifndef DISPLAY_ROWS_H
#define DISPLAY_ROWS_H

#include <stdbool.h>
#include <stdint.h>

#include "app_types.h" /* sensor_sample_t */

typedef struct {
    char label[8];   /* e.g. "TEMP" */
    char unit[6];    /* e.g. "C", "hPa"; empty = no unit */
    float value;     /* engineering value pulled from the sample */
    uint8_t decimals; /* printf precision */
} disp_row_t;

/*
 * Fill out[0..return-1] with the rows for `sensor_type` (a dp_sensor_t) using `s`, appending a BATT
 * row. Returns the row count (<= max). sensor_type == DP_SENSOR_NONE / unknown -> the legacy
 * VOC/PRESS/TEMP set (so behaviour is unchanged until a profile/sensor selects otherwise).
 */
int display_rows_build(uint8_t sensor_type, const sensor_sample_t *s, disp_row_t out[], int max);

#endif /* DISPLAY_ROWS_H */
