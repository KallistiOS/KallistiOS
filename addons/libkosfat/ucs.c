/* KallistiOS ##version##

   ucs.c
   Copyright (C) 2019 Lawrence Sebald
*/

#ifndef FAT_NO_WCTYPE
#include <wctype.h>
#else
#include <ctype.h>
#endif

#include "ucs.h"

int fat_utf8_to_ucs2(uint16_t *out, const uint8_t *in, size_t olen,
                     size_t ilen) {
    size_t i, j;

    for(i = 0, j = 0; i < ilen && j < olen - 1;) {
        if(in[i] <= 0x7F) {
            out[j++] = (uint16_t)in[i++];
        }
        /* Do we have a 2 byte sequence? */
        else if((in[i] & 0xE0) == 0xC0) {
            if(ilen < i + 2)
                return -1;

            out[j] = (uint16_t)((in[i++] & 0x1F) << 6);

            /* Make sure the input has the right bits set. */
            if((in[i] & 0xC0) != 0x80)
                return -1;

            out[j++] |= (uint16_t)(in[i++] & 0x3F);
        }
        /* Do we have a 3 byte sequence? */
        else if((in[i] & 0xF0) == 0xE0) {
            if(ilen < i + 3)
                return -1;

            out[j] = (uint16_t)((in[i++] & 0x0F) << 12);

            /* Make sure the input has the right bits set. */
            if((in[i] & 0xC0) != 0x80)
                return -1;

            out[j] |= (uint16_t)((in[i++] & 0x3F) << 6);

            /* Make sure the input has the right bits set. */
            if((in[i] & 0xC0) != 0x80)
                return -1;

            out[j++] |= (uint16_t)(in[i++] & 0x3F);
        }
        /* 4 byte sequences can't be encoded as UCS-2. */
        else {
            return -1;
        }
    }

    /* Add a NUL terminator and return. */
    out[j] = 0;
    return 0;
}

int fat_ucs2_to_utf8(uint8_t *out, const uint16_t *in, size_t olen,
                     size_t ilen) {
    size_t i, j;

    for(i = 0, j = 0; i < ilen; ++i) {
        if(in[i] <= 0x007F) {
            if(olen < j + 2)
                return -1;

            out[j++] = (uint8_t)in[i];
        }
        else if(in[i] <= 0x07FF) {
            if(olen < j + 3)
                return -1;

            out[j++] = (uint8_t)(0xC0 | ((in[i] >> 6) & 0x1F));
            out[j++] = (uint8_t)(0x80 | (in[i] & 0x3F));
        }
        else {
            if(olen < j + 4)
                return -1;

            out[j++] = (uint8_t)(0xE0 | ((in[i] >> 12) & 0x0F));
            out[j++] = (uint8_t)(0x80 | ((in[i] >> 6) & 0x3F));
            out[j++] = (uint8_t)(0x80 | (in[i] & 0x3F));
        }
    }

    /* Add a NUL terminator and return. */
    out[j] = 0;
    return 0;
}

size_t fat_strlen_ucs2(const uint16_t *in) {
    size_t i = 0;

    while(*in++)
        ++i;

    return i;
}

#ifndef FAT_NO_WCTYPE
void fat_ucs2_tolower(uint16_t *in, size_t len) {
    while(len--) {
        *in = towlower(*in);
        ++in;
    }
}
#else
void fat_ucs2_tolower(uint16_t *in, size_t len) {
    /* This version only works for ASCII, for now. */
    while(len--) {
        if(*in < 0x00FF) {
            *in = tolower(*in);
        }

        ++in;
    }
}
#endif
