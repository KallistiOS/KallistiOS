/* KallistiOS ##version##

   bitops.c
   Copyright (C) 2012 Lawrence Sebald
*/

#include <stdint.h>

uint32_t ext2_bit_find_nonzero(const uint32_t *btbl, uint32_t start,
                               uint32_t end) {
    uint32_t i, j;
    uint32_t tmp;

    i = start >> 5;
    j = start & 0x1F;

    while((end >> 5) > i) {
        tmp = btbl[i];
        if(tmp != 0) {
            for(; j < 32; ++j) {
                if(tmp & (1u << j))
                    return (i << 5) | j;
            }
        }

        j = 0;
        ++i;
    }

    if((end >> 5) == i && (end & 0x1F)) {
        tmp = btbl[i];
        if(tmp != 0) {
            for(; j < (end & 0x1F); ++j) {
                if(tmp & (1u << j))
                    return (i << 5) | j;
            }
        }
    }

    return end + 1;
}

uint32_t ext2_bit_find_zero(const uint32_t *btbl, uint32_t start,
                            uint32_t end) {
    uint32_t i, j;
    uint32_t tmp;

    i = start >> 5;
    j = start & 0x1F;

    while((end >> 5) > i) {
        tmp = btbl[i];
        if(tmp != 0xFFFFFFFF) {
            for(; j < 32; ++j) {
                if(!(tmp & (1u << j)))
                    return (i << 5) | j;
            }
        }

        j = 0;
        ++i;
    }

    if((end >> 5) == i && (end & 0x1F)) {
        tmp = btbl[i];
        if(tmp != 0xFFFFFFFF) {
            for(; j < (end & 0x1F); ++j) {
                if(!(tmp & (1u << j)))
                    return (i << 5) | j;
            }
        }
    }

    return end + 1;
}
