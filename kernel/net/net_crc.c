/* KallistiOS ##version##

   kernel/net/net_crc.c
   Copyright (C) 2009, 2010, 2012 Lawrence Sebald

*/

#include <kos/net.h>

/* Calculate a CRC-32 checksum over a given block of data. Somewhat inspired by
   the CRC32 function in Figure 14-6 of http://www.hackersdelight.org/crc.pdf */
uint32_t net_crc32le(const uint8_t *data, int size) {
    int i;
    uint32_t rv = 0xFFFFFFFF;

    for(i = 0; i < size; ++i) {
        rv ^= data[i];
        rv = (0xEDB88320 & (-(rv & 1))) ^(rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^(rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^(rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^(rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^(rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^(rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^(rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^(rv >> 1);
    }

    return ~rv;
}

/* This one isn't quite as nice as the one above for little-endian... */
uint32_t net_crc32be(const uint8_t *data, int size) {
    int i, j;
    uint32_t rv = 0xFFFFFFFF, b, c;

    for(i = 0; i < size; ++i) {
        b = data[i];

        for(j = 0; j < 8; ++j) {
            c = ((rv & 0x80000000) ? 1 : 0) ^(b & 1);
            b >>= 1;

            if(c)   rv = ((rv << 1) ^ 0x04C11DB6) | c;
            else    rv <<= 1;
        }
    }

    return rv;
}

/* Based on code found at: http://www.ccsinfo.com/forum/viewtopic.php?t=24977 */
uint16_t net_crc16ccitt(const uint8_t *data, int size, uint16_t start) {
    uint16_t rv = start, tmp;

    while(size--) {
        tmp = (rv >> 8) ^ *data++;
        tmp ^= tmp >> 4;

        rv = (rv << 8) ^ (tmp << 12) ^ (tmp << 5) ^ tmp;
    }

    return rv;
}
