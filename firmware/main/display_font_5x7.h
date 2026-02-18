#pragma once

#include <stdbool.h>
#include <stdint.h>

static inline bool display_font_5x7_get(char c, uint8_t out[5])
{
    if (out == NULL) {
        return false;
    }

    if (c >= 'a' && c <= 'z') {
        c = (char)(c - ('a' - 'A'));
    }

    switch (c) {
    case ' ':
        out[0] = 0x00; out[1] = 0x00; out[2] = 0x00; out[3] = 0x00; out[4] = 0x00; return true;
    case '-':
        out[0] = 0x08; out[1] = 0x08; out[2] = 0x08; out[3] = 0x08; out[4] = 0x08; return true;
    case '.':
        out[0] = 0x00; out[1] = 0x60; out[2] = 0x60; out[3] = 0x00; out[4] = 0x00; return true;
    case ':':
        out[0] = 0x00; out[1] = 0x36; out[2] = 0x36; out[3] = 0x00; out[4] = 0x00; return true;
    case '/':
        out[0] = 0x20; out[1] = 0x10; out[2] = 0x08; out[3] = 0x04; out[4] = 0x02; return true;
    case '0':
        out[0] = 0x3E; out[1] = 0x51; out[2] = 0x49; out[3] = 0x45; out[4] = 0x3E; return true;
    case '1':
        out[0] = 0x00; out[1] = 0x42; out[2] = 0x7F; out[3] = 0x40; out[4] = 0x00; return true;
    case '2':
        out[0] = 0x62; out[1] = 0x51; out[2] = 0x49; out[3] = 0x49; out[4] = 0x46; return true;
    case '3':
        out[0] = 0x22; out[1] = 0x41; out[2] = 0x49; out[3] = 0x49; out[4] = 0x36; return true;
    case '4':
        out[0] = 0x18; out[1] = 0x14; out[2] = 0x12; out[3] = 0x7F; out[4] = 0x10; return true;
    case '5':
        out[0] = 0x2F; out[1] = 0x49; out[2] = 0x49; out[3] = 0x49; out[4] = 0x31; return true;
    case '6':
        out[0] = 0x3E; out[1] = 0x49; out[2] = 0x49; out[3] = 0x49; out[4] = 0x32; return true;
    case '7':
        out[0] = 0x01; out[1] = 0x71; out[2] = 0x09; out[3] = 0x05; out[4] = 0x03; return true;
    case '8':
        out[0] = 0x36; out[1] = 0x49; out[2] = 0x49; out[3] = 0x49; out[4] = 0x36; return true;
    case '9':
        out[0] = 0x26; out[1] = 0x49; out[2] = 0x49; out[3] = 0x49; out[4] = 0x3E; return true;
    case 'A':
        out[0] = 0x7E; out[1] = 0x09; out[2] = 0x09; out[3] = 0x09; out[4] = 0x7E; return true;
    case 'B':
        out[0] = 0x7F; out[1] = 0x49; out[2] = 0x49; out[3] = 0x49; out[4] = 0x36; return true;
    case 'C':
        out[0] = 0x3E; out[1] = 0x41; out[2] = 0x41; out[3] = 0x41; out[4] = 0x22; return true;
    case 'D':
        out[0] = 0x7F; out[1] = 0x41; out[2] = 0x41; out[3] = 0x22; out[4] = 0x1C; return true;
    case 'E':
        out[0] = 0x7F; out[1] = 0x49; out[2] = 0x49; out[3] = 0x49; out[4] = 0x41; return true;
    case 'F':
        out[0] = 0x7F; out[1] = 0x09; out[2] = 0x09; out[3] = 0x09; out[4] = 0x01; return true;
    case 'G':
        out[0] = 0x3E; out[1] = 0x41; out[2] = 0x49; out[3] = 0x49; out[4] = 0x7A; return true;
    case 'H':
        out[0] = 0x7F; out[1] = 0x08; out[2] = 0x08; out[3] = 0x08; out[4] = 0x7F; return true;
    case 'I':
        out[0] = 0x00; out[1] = 0x41; out[2] = 0x7F; out[3] = 0x41; out[4] = 0x00; return true;
    case 'J':
        out[0] = 0x20; out[1] = 0x40; out[2] = 0x41; out[3] = 0x3F; out[4] = 0x01; return true;
    case 'K':
        out[0] = 0x7F; out[1] = 0x08; out[2] = 0x14; out[3] = 0x22; out[4] = 0x41; return true;
    case 'L':
        out[0] = 0x7F; out[1] = 0x40; out[2] = 0x40; out[3] = 0x40; out[4] = 0x40; return true;
    case 'M':
        out[0] = 0x7F; out[1] = 0x02; out[2] = 0x0C; out[3] = 0x02; out[4] = 0x7F; return true;
    case 'N':
        out[0] = 0x7F; out[1] = 0x04; out[2] = 0x08; out[3] = 0x10; out[4] = 0x7F; return true;
    case 'O':
        out[0] = 0x3E; out[1] = 0x41; out[2] = 0x41; out[3] = 0x41; out[4] = 0x3E; return true;
    case 'P':
        out[0] = 0x7F; out[1] = 0x09; out[2] = 0x09; out[3] = 0x09; out[4] = 0x06; return true;
    case 'Q':
        out[0] = 0x3E; out[1] = 0x41; out[2] = 0x51; out[3] = 0x21; out[4] = 0x5E; return true;
    case 'R':
        out[0] = 0x7F; out[1] = 0x09; out[2] = 0x19; out[3] = 0x29; out[4] = 0x46; return true;
    case 'S':
        out[0] = 0x46; out[1] = 0x49; out[2] = 0x49; out[3] = 0x49; out[4] = 0x31; return true;
    case 'T':
        out[0] = 0x01; out[1] = 0x01; out[2] = 0x7F; out[3] = 0x01; out[4] = 0x01; return true;
    case 'U':
        out[0] = 0x3F; out[1] = 0x40; out[2] = 0x40; out[3] = 0x40; out[4] = 0x3F; return true;
    case 'V':
        out[0] = 0x1F; out[1] = 0x20; out[2] = 0x40; out[3] = 0x20; out[4] = 0x1F; return true;
    case 'W':
        out[0] = 0x7F; out[1] = 0x20; out[2] = 0x18; out[3] = 0x20; out[4] = 0x7F; return true;
    case 'X':
        out[0] = 0x63; out[1] = 0x14; out[2] = 0x08; out[3] = 0x14; out[4] = 0x63; return true;
    case 'Y':
        out[0] = 0x03; out[1] = 0x04; out[2] = 0x78; out[3] = 0x04; out[4] = 0x03; return true;
    case 'Z':
        out[0] = 0x61; out[1] = 0x51; out[2] = 0x49; out[3] = 0x45; out[4] = 0x43; return true;
    default:
        out[0] = 0x7F; out[1] = 0x41; out[2] = 0x5D; out[3] = 0x41; out[4] = 0x7F; return false;
    }
}

