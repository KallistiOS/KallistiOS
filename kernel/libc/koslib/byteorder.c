/* KallistiOS ##version##

   byteorder.c
   Copyright (C) 2001 Megan Potter
   Copyright (C) 2016 Lawrence Sebald

*/

/* Byte-order translation functions */
#include <stdint.h>
#include <arch/byteorder.h>

/* Network to Host short */
uint16_t ntohs(uint16_t value) {
    return arch_ntohs(value);
}

/* Network to Host long */
uint32_t ntohl(uint32_t value) {
    return arch_ntohl(value);
}

/* Host to Network short */
uint32_t htons(uint32_t value) {
    return arch_htons(value);
}

/* Host to Network long */
uint32_t htonl(uint32_t value) {
    return arch_htonl(value);
}
