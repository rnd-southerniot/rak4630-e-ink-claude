#include <stdio.h>

#include "gate_id_map.h"

typedef struct {
    int in;
    int out;
} map_case_t;

int main(void)
{
    static const map_case_t cases[] = {
        {0, 0},
        {1, 1},
        {2, 3},
        {3, 4},
        {4, 5},
        {5, 6},
        {6, 7},
        {7, 8},
        {8, 9},
    };

    for (size_t i = 0; i < (sizeof(cases) / sizeof(cases[0])); i++) {
        const int got = app_gate_translate_legacy_id(cases[i].in);
        if (got != cases[i].out) {
            fprintf(stderr, "legacy map mismatch in=%d got=%d expected=%d\n", cases[i].in, got, cases[i].out);
            return 1;
        }
    }

    if (app_gate_translate_legacy_id(-1) != -1 || app_gate_translate_legacy_id(9) != -1) {
        fprintf(stderr, "legacy invalid map mismatch\n");
        return 1;
    }

    printf("PASS gate_id_legacy_map deterministic vector\n");
    return 0;
}

