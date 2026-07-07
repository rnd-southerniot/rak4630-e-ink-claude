/*
 * telemetry.h — ADR-005 payload schema constants + senseflow I2C device-type registry.
 *
 * Pure C, no SDK dependency (the device_profile core + host tests build without ESP-IDF).
 * Shared 3-byte header [schema][device-type][flags] with careflow-modbus-node; senseflow I2C
 * sensors use the 0x10+ device-byte range (careflow Modbus meters use 0x01..0x0F) so a future
 * unified fleet decoder can tell products apart.
 */
#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>

/* Device type — payload byte 1. */
typedef enum {
    TELEMETRY_DEV_BME280 = 0x10, /* Bosch temp/pressure(/humidity) */
    TELEMETRY_DEV_SGP40 = 0x11,  /* Sensirion VOC index */
    TELEMETRY_DEV_SHTC3 = 0x12,  /* Sensirion temp/humidity */
} telemetry_device_t;

/* Payload schema version — payload byte 0. Bump on ANY field/layout/scaling change. */
#define TELEMETRY_SCHEMA_VERSION 0x01u

/* Common header length: [schema][device-type][flags]. */
#define TELEMETRY_HEADER_LEN 3u

/* Flags — payload byte 2 (bitfield). */
#define TELEMETRY_FLAG_SIMULATED 0x01u /* values synthesized, not read from a real sensor */
#define TELEMETRY_FLAG_STALE 0x02u     /* last sensor read failed; values may be stale/zero */

#endif /* TELEMETRY_H */
