/* KallistiOS ##version##

   arch/dreamcast/include/arch/byteorder.h
   Copyright (C) 2015 Lawrence Sebald

*/

#ifndef __ARCH_BYTEORDER_H
#define __ARCH_BYTEORDER_H

#include <kos/cdefs.h>
__BEGIN_DECLS

/* Bring in the newlib header that defines the BYTE_ORDER macro */
#include <machine/endian.h>

__depr("arch_swap16() is deprecated, use __builtin_bswap16().")
static inline uint16_t arch_swap16(uint16_t x) {
    return __builtin_bswap16(x);
}

__depr("arch_swap32() is deprecated, use __builtin_bswap32().")
static inline uint32_t arch_swap32(uint32_t x) {
    return __builtin_bswap32(x);
}

__depr("arch_ntohs() is deprecated, use ntohs() from <arpa/inet.h>")
static inline uint16_t arch_ntohs(uint16_t x) {
    return __builtin_bswap16(x);
}

__depr("arch_ntohl() is deprecated, use ntohl() from <arpa/inet.h>")
static inline uint32_t arch_ntohl(uint32_t x) {
    return __builtin_bswap32(x);
}

__depr("arch_htons() is deprecated, use htons() from <arpa/inet.h>")
static inline uint16_t arch_htons(uint16_t x) {
    return __builtin_bswap16(x);
}

__depr("arch_htonl() is deprecated, use htonl() from <arpa/inet.h>")
static inline uint32_t arch_htonl(uint32_t x) {
    return __builtin_bswap32(x);
}

__END_DECLS

#endif /* !__ARCH_BYTEORDER_H */
