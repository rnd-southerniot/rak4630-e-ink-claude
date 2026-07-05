/*
 * device_profile_test.c — host test for the multi-bus (v2) device_profile core with an I2C
 * BME280-shaped profile: dp_serialize/dp_deserialize roundtrip (incl. bus_kind/i2c_addr/sensor_type),
 * dp_encode_payload exact bytes, and dp_simulate determinism. Pure C; prints PASS or returns nonzero.
 *   cc -I ../../firmware/components/device_profile/include device_profile_test.c \
 *      ../../firmware/components/device_profile/device_profile.c -lm -o /tmp/dpt && /tmp/dpt
 */
#include "device_profile.h"
#include "telemetry.h"

#include <stdio.h>
#include <string.h>

static int fails = 0;
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c);                                    \
            ++fails;                                                                               \
        }                                                                                          \
    } while (0)

static int approx(float a, float b) { return (a - b) < 0.02f && (b - a) < 0.02f; }

/* BME280 over I2C: 3 measurands (temp/pressure/humidity) -> 9-byte ADR-005 payload. */
static const dp_measurand_t MEAS[] = {
    /* reg/fc/word unused for I2C (compiled driver produces values); type/scale placeholder. */
    {0, 0, DP_F32, DP_ABCD, 1.0f, 0.0f}, /* 0 temp_c */
    {0, 0, DP_F32, DP_ABCD, 1.0f, 0.0f}, /* 1 pressure_hpa */
    {0, 0, DP_F32, DP_ABCD, 1.0f, 0.0f}, /* 2 humidity_pct */
};
static const dp_field_t FIELDS[] = {
    {0, 0, DP_ENC_I16, 100.0f}, /* temp x100  (-40..85 C) */
    {1, 2, DP_ENC_U16, 10.0f},  /* pressure x10 hPa */
    {2, 4, DP_ENC_U16, 100.0f}, /* humidity x100 % */
};
static const device_profile_t BME280 = {
    .device_byte = TELEMETRY_DEV_BME280, /* 0x10 */
    .bus_kind = DP_BUS_I2C,
    .i2c_addr = 0x76,
    .sensor_type = DP_SENSOR_BME280,
    .n_meas = 3,
    .meas = MEAS,
    .total_len = 9, /* 3 header + 6 body */
    .n_fields = 3,
    .fields = FIELDS,
};

static void test_encode(void)
{
    const float v[3] = {23.45f, 1013.2f, 55.10f};
    uint8_t out[32] = {0};
    const size_t n = dp_encode_payload(&BME280, v, TELEMETRY_FLAG_STALE, out, sizeof(out));
    CHECK(n == 9u);
    const uint8_t expect[9] = {
        0x01, 0x10, 0x02,       /* schema, device 0x10 BME280, flags=STALE */
        0x09, 0x29,             /* temp 23.45 x100 = 2345 */
        0x27, 0x94,             /* pressure 1013.2 x10 = 10132 */
        0x15, 0x86,             /* humidity 55.10 x100 = 5510 */
    };
    CHECK(memcmp(out, expect, sizeof(expect)) == 0);

    /* negative temp encodes as two's-complement i16: -5.00 x100 = -500 = 0xFE0C */
    const float v2[3] = {-5.0f, 0, 0};
    (void)dp_encode_payload(&BME280, v2, 0, out, sizeof(out));
    CHECK(out[3] == 0xFE && out[4] == 0x0C);
}

static void test_blob_roundtrip(void)
{
    const size_t want = DP_BLOB_HEADER + 3u * DP_BLOB_MEAS_REC + 3u * DP_BLOB_FIELD_REC + DP_BLOB_CRC;
    CHECK(dp_blob_size(&BME280) == want); /* 21 + 39 + 21 + 2 = 83 */

    uint8_t blob[256] = {0};
    const size_t n = dp_serialize(&BME280, blob, sizeof(blob));
    CHECK(n == want);
    CHECK(blob[0] == DP_BLOB_VERSION); /* v2 */

    dp_profile_storage_t store;
    CHECK(dp_deserialize(blob, n, &store));
    const device_profile_t *q = &store.profile;
    CHECK(q->device_byte == 0x10 && q->bus_kind == DP_BUS_I2C && q->i2c_addr == 0x76 &&
          q->sensor_type == DP_SENSOR_BME280);
    CHECK(q->total_len == 9 && q->n_meas == 3 && q->n_fields == 3);
    CHECK(q->fields[1].value_index == 1 && q->fields[1].offset == 2 &&
          q->fields[1].enc == DP_ENC_U16 && approx(q->fields[1].scale, 10.0f));

    /* deserialized profile encodes identical bytes */
    const float v[3] = {23.45f, 1013.2f, 55.10f};
    uint8_t a[32] = {0}, b[32] = {0};
    const size_t la = dp_encode_payload(&BME280, v, 0, a, sizeof(a));
    const size_t lb = dp_encode_payload(q, v, 0, b, sizeof(b));
    CHECK(la == lb && la == 9 && memcmp(a, b, la) == 0);

    /* a v1 blob (wrong version byte) is rejected */
    blob[0] = 1u;
    CHECK(!dp_deserialize(blob, n, &store));
}

static void test_simulate(void)
{
    float v0[DP_MAX_MEAS], v0b[DP_MAX_MEAS];
    dp_simulate(0, &BME280, v0, DP_MAX_MEAS);
    dp_simulate(0, &BME280, v0b, DP_MAX_MEAS);
    CHECK(v0[0] == v0b[0] && v0[1] == v0b[1]); /* deterministic */
    /* tick-0 first field: nominal(i16=3000)/scale(100) = 30.0 exactly */
    CHECK(approx(v0[0], 30.0f));
    /* encodes without saturating (all fields non-zero, valid frame) */
    uint8_t out[32];
    CHECK(dp_encode_payload(&BME280, v0, TELEMETRY_FLAG_SIMULATED, out, sizeof(out)) == 9u);
    /* n_values < n_meas rejected (no write) */
    float small[2] = {1, 2};
    dp_simulate(0, &BME280, small, 2);
    CHECK(small[0] == 1 && small[1] == 2);
}

int main(void)
{
    test_encode();
    test_blob_roundtrip();
    test_simulate();
    if (fails == 0) {
        printf("PASS device_profile_test (multi-bus v2, BME280/I2C)\n");
        return 0;
    }
    printf("FAILED %d checks\n", fails);
    return 1;
}
