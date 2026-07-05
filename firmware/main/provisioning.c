/*
 * provisioning.c — on-device provisioning commands (senseflow, P3).
 *
 * prov-lorawan/-show/-clear/-done write the NVS 'prov' namespace additively (OTAA credentials),
 * preserving the 'lorawan' nonces/session, then restart into field mode. Driven by hand or by
 * tools/provision_creds.py. (prov-profile for the I2C device-profile blob lands in P4.)
 *
 * The single USB-Serial-JTAG REPL is owned by prov_console.cpp (which also hosts the CRM-contract
 * `prov creds|show` commands); this module only registers its commands onto it.
 */
#include "provisioning.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_console.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"

static const char *TAG = "prov";
#define PROV_NS "prov"

static bool parse_hex_bytes(const char *s, uint8_t *out, size_t nbytes)
{
    if (strlen(s) != nbytes * 2) {
        return false;
    }
    for (size_t i = 0; i < nbytes; ++i) {
        char b[3] = {s[2 * i], s[2 * i + 1], 0};
        char *end = NULL;
        long v = strtol(b, &end, 16);
        if (end != b + 2 || v < 0 || v > 0xff) {
            return false;
        }
        out[i] = (uint8_t)v;
    }
    return true;
}

/* prov-lorawan <deveui16hex> <joineui16hex> <appkey32hex> */
static int cmd_lorawan(int argc, char **argv)
{
    if (argc != 4 || strlen(argv[1]) != 16 || strlen(argv[2]) != 16 || strlen(argv[3]) != 32) {
        printf("ERR usage: prov-lorawan <deveui16hex> <joineui16hex> <appkey32hex>\n");
        return 1;
    }
    uint8_t appkey[16];
    if (!parse_hex_bytes(argv[3], appkey, sizeof(appkey))) {
        printf("ERR appkey not 32 hex chars\n");
        return 1;
    }
    const uint64_t de = strtoull(argv[1], NULL, 16);
    const uint64_t je = strtoull(argv[2], NULL, 16);
    nvs_handle_t h;
    if (nvs_open(PROV_NS, NVS_READWRITE, &h) != ESP_OK) {
        printf("ERR nvs open\n");
        return 1;
    }
    nvs_set_u64(h, "deveui", de);
    nvs_set_u64(h, "joineui", je);
    nvs_set_blob(h, "appkey", appkey, sizeof(appkey));
    esp_err_t err = nvs_commit(h);
    nvs_close(h);
    printf(err == ESP_OK ? "OK lorawan deveui=%016llx joineui=%016llx appkey=<redacted>\n"
                         : "ERR nvs commit\n",
           (unsigned long long)de, (unsigned long long)je);
    return err == ESP_OK ? 0 : 1;
}

static int cmd_show(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    nvs_handle_t h;
    if (nvs_open(PROV_NS, NVS_READONLY, &h) != ESP_OK) {
        printf("prov: (empty)\n");
        return 0;
    }
    uint64_t de = 0, je = 0;
    nvs_get_u64(h, "deveui", &de);
    nvs_get_u64(h, "joineui", &je);
    uint8_t ak[16];
    size_t n = sizeof(ak);
    const bool hasak = (nvs_get_blob(h, "appkey", ak, &n) == ESP_OK && n == sizeof(ak));
    nvs_close(h);
    printf("prov: deveui=%016llx joineui=%016llx appkey=%s\n", (unsigned long long)de,
           (unsigned long long)je, hasak ? "set" : "unset");
    return 0;
}

static int cmd_clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    nvs_handle_t h;
    if (nvs_open(PROV_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_erase_all(h);
        nvs_commit(h);
        nvs_close(h);
    }
    printf("OK cleared\n");
    return 0;
}

static int cmd_done(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("OK restarting into field mode\n");
    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_restart();
    return 0;
}

/* Register the prov-* commands onto the REPL created + owned by prov_console_init(). */
void provisioning_register_commands(void)
{
    const esp_console_cmd_t cmds[] = {
        {.command = "prov-lorawan",
         .help = "<deveui16> <joineui16> <appkey32> — set OTAA credentials",
         .func = cmd_lorawan},
        {.command = "prov-show",
         .help = "print current provisioning (appkey redacted)",
         .func = cmd_show},
        {.command = "prov-clear", .help = "erase the provisioning namespace", .func = cmd_clear},
        {.command = "prov-done", .help = "commit + restart into field mode", .func = cmd_done},
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); ++i) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }
    ESP_LOGI(TAG, "prov-* commands registered (prov-lorawan/-show/-clear/-done; "
                  "or tools/provision_creds.py); nonces/session preserved");
}
