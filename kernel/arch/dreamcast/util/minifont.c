/* KallistiOS ##version##

   util/minifont.c
   Copyright (C) 2020 Lawrence Sebald

*/

#include <string.h>
#include <dc/minifont.h>
#include "minifont.h"

int minifont_draw(uint16_t *buffer, uint32_t bufwidth, char c) {
    uint16_t pos;
    uint8_t byte, i, j, k;
    uint16_t *cur;

    /* Treat other characters as a space */
    if(c <= ' ' || c > '~')
        return char_width;

    pos = (c - char_start) * ((char_width / 8) * char_height);

    for(i = 0; i < char_height; ++i) {
        cur = buffer;

        for(j = 0; j < char_width / 8; ++j) {
            byte = minifont_data[pos + (i * (char_width / 8)) + j];

            for(k = 0; k < 8; ++k) {
                if(byte & (1 << (7 - k)))
                    *cur++ = 0xFFFF;
                else
                    ++cur;
            }
        }

        buffer += bufwidth;
    }

    return char_width;
}

int minifont_draw_str(uint16_t *buffer, uint32_t bufwidth, const char *str) {
    char c;
    int adv = 0;

    while((c = *str++)) {
        adv += minifont_draw(buffer + adv, bufwidth, c);
    }

    return adv;
}
