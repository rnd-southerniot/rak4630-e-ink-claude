#pragma once

/*
 * ESP-IDF compatibility shim. On nRF52 (no ESP-IDF) it defines esp_err_t and the
 * error codes; on ESP32 the real ESP-IDF provides them, so we use those. The
 * logging macros use our Serial.printf "[L] tag: ..." format on BOTH boards so
 * the gate markers (result=PASS, handshake=...) parse identically everywhere.
 */

#include <stdint.h>

#if defined(ESP_PLATFORM)
/* ESP32: real esp_err_t, ESP_OK/ESP_FAIL/ESP_ERR_*, esp_err_to_name, ESP_ERROR_CHECK. */
#include <esp_err.h>
#else
#ifdef __cplusplus
extern "C" {
#endif

typedef int esp_err_t;

#define ESP_OK                   0
#define ESP_FAIL                (-1)
#define ESP_ERR_INVALID_ARG     (-2)
#define ESP_ERR_INVALID_STATE   (-3)
#define ESP_ERR_NO_MEM          (-4)
#define ESP_ERR_NOT_FOUND       (-5)
#define ESP_ERR_TIMEOUT         (-6)
#define ESP_ERR_INVALID_CRC     (-7)
#define ESP_ERR_INVALID_RESPONSE (-8)

static inline const char *esp_err_to_name(esp_err_t err)
{
    switch (err) {
    case ESP_OK:                  return "OK";
    case ESP_FAIL:                return "FAIL";
    case ESP_ERR_INVALID_ARG:     return "INVALID_ARG";
    case ESP_ERR_INVALID_STATE:   return "INVALID_STATE";
    case ESP_ERR_NO_MEM:          return "NO_MEM";
    case ESP_ERR_NOT_FOUND:       return "NOT_FOUND";
    case ESP_ERR_TIMEOUT:         return "TIMEOUT";
    case ESP_ERR_INVALID_CRC:     return "INVALID_CRC";
    case ESP_ERR_INVALID_RESPONSE: return "INVALID_RESPONSE";
    default:                      return "UNKNOWN";
    }
}

#define ESP_ERROR_CHECK(x) do { \
    esp_err_t __err = (x); \
    if (__err != ESP_OK) { \
        Serial.printf("[E] ESP_ERROR_CHECK failed: %s at %s:%d\r\n", \
                      esp_err_to_name(__err), __FILE__, __LINE__); \
        while(1) { delay(1000); } \
    } \
} while(0)

#ifdef __cplusplus
}
#endif
#endif /* ESP_PLATFORM */

/* Logging — our Serial.printf format on both boards (override ESP-IDF's if present). */
#ifdef ESP_LOGI
#undef ESP_LOGI
#endif
#ifdef ESP_LOGW
#undef ESP_LOGW
#endif
#ifdef ESP_LOGE
#undef ESP_LOGE
#endif

#ifndef LOG_TAG_LETTER
#define LOG_TAG_LETTER(letter, tag, fmt, ...) \
    do { Serial.printf("[%c] %s: " fmt "\r\n", letter, tag, ##__VA_ARGS__); } while(0)
#endif

#define ESP_LOGI(tag, fmt, ...)  LOG_TAG_LETTER('I', tag, fmt, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...)  LOG_TAG_LETTER('W', tag, fmt, ##__VA_ARGS__)
#define ESP_LOGE(tag, fmt, ...)  LOG_TAG_LETTER('E', tag, fmt, ##__VA_ARGS__)

#ifdef ESP_RETURN_ON_ERROR
#undef ESP_RETURN_ON_ERROR
#endif
#define ESP_RETURN_ON_ERROR(x, tag, msg) do { \
    esp_err_t __err = (x); \
    if (__err != ESP_OK) { \
        ESP_LOGE(tag, "%s: %s", msg, esp_err_to_name(__err)); \
        return __err; \
    } \
} while(0)
