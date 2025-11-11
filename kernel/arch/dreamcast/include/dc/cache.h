/* KallistiOS ##version##

   arch/dreamcast/include/dc/cache.h
   Copyright (C) 2025 Paul Cercueil
   Copyright (C) 2025 Matt Slevinsky
   Copyright (C) 2025 TapamN
*/

/** \file    dc/cache.h
    \brief   Cache management functionality.
    \ingroup system_cache

    This file contains definitions and low-level routines to manipulate the
    instruction and data caches.

    \author Paul Cercueil
    \author Matt Slevinsky
    \author TapamN
*/

#ifndef __DC_CACHE_H
#define __DC_CACHE_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <kos/regfield.h>
#include <stdbool.h>
#include <stdint.h>

#define CCR_OCE         BIT(0)  /** \< OC enable */
#define CCR_WT          BIT(1)  /** \< Write-through enable (for P0/U0/P3 in non-MMU mode) */
#define CCR_CB          BIT(2)  /** \< Copy-back enable for P1 area */
#define CCR_OCI         BIT(3)  /** \< OC invalidate */
#define CCR_ORA         BIT(5)  /** \< OC RAM enable */
#define CCR_OIX         BIT(7)  /** \< OC INDEX enable */
#define CCR_ICE         BIT(8)  /** \< IC enable (in non-MMU mode) */
#define CCR_ICI         BIT(11) /** \< IC invalidate */
#define CCR_IIX         BIT(15) /** \< IC INDEX enable */

/** \brief  Write CCR register.

    This function is used internally to write the cache control register.

    \param  mask            Mask of the bits to update
    \param  value           Value to replace the masked bits with
*/
void cache_write_ccr(uint32_t mask, uint32_t value);

/** \brief  Enable or disable OCINDEX mode.

    This function can be used to enable or disable OCINDEX mode.

    In OCINDEX mode, the bits 25 and 12-5 are used as the data cache line index.
    In non-OCINDEX mode, the bits 13-5 are used as the data cache line index.

    Using OCINDEX, data memory-mapped to an address with bit 25 set
    (e.g. 0x2000000) will always stay in cache (unless its cache line is
    reused by another address with bit 25 set), as the RAM size on supported
    SH4 platforms is lower than (1 << 25), meaning that for common RAM
    addresses, bit 25 is always cleared.

    \param  enable          Whether or not to enable OCINDEX mode
*/
static inline void dcache_toggle_ocindex(bool enable) {
    cache_write_ccr(CCR_OIX, enable ? CCR_OIX : 0);
}

/** \brief  Enable or disable OCRAM mode.

    This function can be used to enable or disable OCRAM mode.

    In OCRAM mode, half the cache can be used as a scratchpad.

    In OCRAM mode, the bits 13 and 11-5 are used as the cache line index.
    In non-OCRAM mode, the bits 13-5 are used as the cache line index.

    Using OCRAM mode, the cache lines 128 to 255 and 384 to 511 are guaranteed
    to be unused, and can be accessed directly as if it was RAM. The first 4 KiB
    can be accessed at address 0xf4001000, the second 4 KiB can be accessed at
    address 0xf4003000.

    \param  enable          Whether or not to enable OCINDEX mode
*/
static inline void dcache_toggle_ocram(bool enable) {
    cache_write_ccr(CCR_ORA, enable ? CCR_ORA : 0);
}

/** \brief  Enable or disable ICINDEX mode.

    This function can be used to enable or disable ICINDEX mode.

    In ICINDEX mode, the bits 25 and 11-5 are used as the instruction cache line
    index. In non-ICINDEX mode, the bits 12-5 are used as the instruction cache
    line index.

    Using ICINDEX, code memory-mapped to an address with bit 25 set
    (e.g. 0x2000000) will always stay in cache (unless its cache line is
    reused by another address with bit 25 set), as the RAM size on supported
    SH4 platforms is lower than (1 << 25), meaning that for common RAM
    addresses, bit 25 is always cleared.

    \param  enable          Whether or not to enable ICINDEX mode
*/
static inline void icache_toggle_icindex(bool enable) {
    cache_write_ccr(CCR_IIX, (enable ? CCR_IIX : 0) | CCR_ICI);
}

__END_DECLS

#endif  /* __DC_CACHE_H */
