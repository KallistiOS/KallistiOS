/* KallistiOS ##version##

   kos/numeric.h
   Copyright (C) 2023 Falco Girgis

*/

/** \file   kos/numeric.h
    \brief  Numeric Algorithms, Conversions, and Utilities

    \author Falco Girgis
*/

#ifndef __KOS_NUMERIC_H
#define __KOS_NUMERIC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <stddef.h>

#define GENMASK(h, l) \
    (((unsigned int)-1 << (l)) & ((unsigned int)-1 >> (31 - (h))))



static inline uint8_t bcd_from_dec(uint8_t dec) {
   return (((dec / 10) << 4) | (dec % 10));
}

static inline uint8_t bcd_to_dec(uint8_t bcd) {
   return (((bcd >> 4) * 10) + (bcd & 0xf));
}

static inline size_t align_to(size_t address, size_t alignment) {
    return (address + (alignment - 1)) & ~(alignment - 1);
}

static inline uint32_t round_up_to_pow2(uint32_t value, uint32_t round_to) {
   return (value + (round_to - 1)) & ~(round_to - 1);
}

static inline round_up_to(uint32_t value, uint32_t round_to) { 
   return ((value + round_to - 1) / round_to) * round_to;
}

/* CRC calculation: calculates the CRC on a VMU file to be written out */
static inline uint16_t crc16(const void *buf, size_t size) {
    int i, c, n;
    const uint8_t *bytes = (const uint8_t *)buf;

    for(i = 0, n = 0; i < size; i++) {
        n ^= (bytes[i] << 8);

        for(c = 0; c < 8; c++) {
            if(n & 0x8000)
                n = (n << 1) ^ 4129;
            else
                n = (n << 1);
        }
    }

    return n & 0xffff;
}
__END_DECLS

#endif /* __KOS_NUMERIC_H */