/* KallistiOS ##version##

   arch/xbox/include/arch/byteorder.h
   Copyright (C) 2026 The KallistiOS Contributors
*/

#ifndef __ARCH_XBOX_BYTEORDER_H
#define __ARCH_XBOX_BYTEORDER_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <machine/endian.h>
#include <stdint.h>

__depr("arch_swap16() is deprecated, use __builtin_bswap16().")
static inline uint16_t arch_swap16(uint16_t value) {
    return __builtin_bswap16(value);
}

__depr("arch_swap32() is deprecated, use __builtin_bswap32().")
static inline uint32_t arch_swap32(uint32_t value) {
    return __builtin_bswap32(value);
}

__depr("arch_ntohs() is deprecated, use ntohs() from <arpa/inet.h>")
static inline uint16_t arch_ntohs(uint16_t value) {
    return __builtin_bswap16(value);
}

__depr("arch_ntohl() is deprecated, use ntohl() from <arpa/inet.h>")
static inline uint32_t arch_ntohl(uint32_t value) {
    return __builtin_bswap32(value);
}

__depr("arch_htons() is deprecated, use htons() from <arpa/inet.h>")
static inline uint16_t arch_htons(uint16_t value) {
    return __builtin_bswap16(value);
}

__depr("arch_htonl() is deprecated, use htonl() from <arpa/inet.h>")
static inline uint32_t arch_htonl(uint32_t value) {
    return __builtin_bswap32(value);
}

__END_DECLS

#endif /* __ARCH_XBOX_BYTEORDER_H */
