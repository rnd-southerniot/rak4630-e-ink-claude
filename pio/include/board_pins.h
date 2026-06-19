#pragma once

/*
 * Board pin-map dispatcher. The active board is selected by a -DBOARD_* flag in
 * platformio.ini (one per env). Shared code includes "board_pins.h" and gets the
 * right pin aliases; board specifics live in board_pins_<board>.h.
 */

#if defined(BOARD_RAK3312)
#include "board_pins_rak3312.h"
#elif defined(BOARD_RAK4631)
#include "board_pins_rak4631.h"
#else
#error "No board selected: define BOARD_RAK4631 or BOARD_RAK3312 (see platformio.ini)"
#endif
