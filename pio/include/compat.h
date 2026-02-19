#pragma once

/*
 * ESP-IDF compatibility shim for Arduino/nRF52840 port.
 * Maps esp_err_t and related macros to simple int return codes.
 */

#include <stdint.h>

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

/* Logging macros — map to Serial.printf */
#ifndef LOG_TAG_LETTER
#define LOG_TAG_LETTER(letter, tag, fmt, ...) \
    do { Serial.printf("[%c] %s: " fmt "\r\n", letter, tag, ##__VA_ARGS__); } while(0)
#endif

#define ESP_LOGI(tag, fmt, ...)  LOG_TAG_LETTER('I', tag, fmt, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...)  LOG_TAG_LETTER('W', tag, fmt, ##__VA_ARGS__)
#define ESP_LOGE(tag, fmt, ...)  LOG_TAG_LETTER('E', tag, fmt, ##__VA_ARGS__)

/* Error check macros */
#define ESP_ERROR_CHECK(x) do { \
    esp_err_t __err = (x); \
    if (__err != ESP_OK) { \
        Serial.printf("[E] ESP_ERROR_CHECK failed: %s at %s:%d\r\n", \
                      esp_err_to_name(__err), __FILE__, __LINE__); \
        while(1) { delay(1000); } \
    } \
} while(0)

#define ESP_RETURN_ON_ERROR(x, tag, msg) do { \
    esp_err_t __err = (x); \
    if (__err != ESP_OK) { \
        ESP_LOGE(tag, "%s: %s", msg, esp_err_to_name(__err)); \
        return __err; \
    } \
} while(0)

#ifdef __cplusplus
}
#endif
