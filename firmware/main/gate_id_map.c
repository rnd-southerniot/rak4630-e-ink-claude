#include "gate_id_map.h"

int app_gate_translate_legacy_id(int legacy_gate)
{
    switch (legacy_gate) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 3;
    case 3:
        return 4;
    case 4:
        return 5;
    case 5:
        return 6;
    case 6:
        return 7;
    case 7:
        return 8;
    case 8:
        return 9;
    default:
        return -1;
    }
}
