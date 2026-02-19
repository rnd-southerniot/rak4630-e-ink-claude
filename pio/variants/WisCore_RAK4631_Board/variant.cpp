/*
 * RAK4631 WisBlock Core variant initialization
 * Pin mapping: Arduino pin N == nRF52840 GPIO N (direct 1:1 mapping)
 */

#include "variant.h"
#include "wiring_constants.h"
#include "wiring_digital.h"
#include "nrf.h"

/*
 * Direct 1:1 pin mapping: g_ADigitalPinMap[N] = N
 * This means Arduino pin 13 maps to nRF52840 P0.13, pin 35 to P1.03, etc.
 * nRF52840 GPIO numbering: P0.00-P0.31 = 0-31, P1.00-P1.15 = 32-47
 */
const uint32_t g_ADigitalPinMap[] =
{
     0,  1,  2,  3,  4,  5,  6,  7,  /* P0.00 - P0.07 */
     8,  9, 10, 11, 12, 13, 14, 15,  /* P0.08 - P0.15 */
    16, 17, 18, 19, 20, 21, 22, 23,  /* P0.16 - P0.23 */
    24, 25, 26, 27, 28, 29, 30, 31,  /* P0.24 - P0.31 */
    32, 33, 34, 35, 36, 37, 38, 39,  /* P1.00 - P1.07 */
    40, 41, 42, 43, 44, 45, 46, 47,  /* P1.08 - P1.15 */
};

void initVariant()
{
    /* Turn off LEDs */
    pinMode(PIN_LED1, OUTPUT);
    ledOff(PIN_LED1);

    pinMode(PIN_LED2, OUTPUT);
    ledOff(PIN_LED2);
}
