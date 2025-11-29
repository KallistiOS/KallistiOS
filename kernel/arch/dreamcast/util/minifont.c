/* KallistiOS ##version##

   util/minifont.c
   Copyright (C) 2020 Lawrence Sebald
   Copyright (C) 2025 Daniel Fairchild

*/


#include <string.h>
#include <dc/minifont.h>
#include "minifont.h"

#define CHAR_WIDTH 8
#define CHAR_HEIGHT 16

#define BYTES_PER_CHAR ((CHAR_WIDTH / 8) * CHAR_HEIGHT)

static uint16_t textcolor = 0xFFFF;

void minifont_set_color(uint8_t r, uint8_t g, uint8_t b) {
    textcolor = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b & 0xF8) >> 3;
}

int minifont_draw(uint16 *buffer, uint32 bufwidth, uint32 c) {
    int pos, i, j, k;
    uint8 byte;
    uint16 *cur;

    if (c < 33 || c > 126) return CHAR_WIDTH;

    pos = (c - 33) * BYTES_PER_CHAR;

    for (i = 0; i < CHAR_HEIGHT; ++i) {
        cur = buffer;

        for (j = 0; j < CHAR_WIDTH / 8; ++j) {
            byte = minifont_data[pos + (i * (CHAR_WIDTH / 8)) + j];

            for(k = 0; k < 8; ++k) {
                if(byte & (1 << (7 - k)))
                    *cur++ = textcolor;
                else
                    ++cur;
            }
        }
        buffer += bufwidth;
    }

    return CHAR_WIDTH;
}

int minifont_draw_str(uint16 *buffer, uint32 bufwidth, const char *str) {
    char c;
    int adv = 0;

    while ((c = *str++)) {
        adv += minifont_draw(buffer + adv, bufwidth, c);
    }

    return adv;
}
