/* KallistiOS ##version##

   arch/dreamcast/include/arch/cache.h
   Copyright (C) 2001 Megan Potter
   Copyright (C) 2014, 2016, 2023 Ruslan Rostovtsev
   Copyright (C) 2023 Andy Barajas
   Copyright (C) 2025 Eric Fradella
   Copyright (C) 2026 Falco Girgis
*/

/** \file    arch/cache.h
    \brief   Cache management functionality.
    \ingroup system_cache

    This file contains definitions for functions that manage the cache in the
    Dreamcast, including functions to flush, invalidate, purge, prefetch and
    allocate the caches.

    \author Megan Potter
    \author Ruslan Rostovtsev
    \author Andy Barajas
    \author Falco Girgis
*/

/* Keep this include above the macro guards */
#include <kos/cache.h>

#ifndef __ARCH_CACHE_H
#define __ARCH_CACHE_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <kos/regfield.h>

#include <stdalign.h>
#include <stdint.h>

#define ARCH_CACHE_L1_ICACHE_SIZE       (8 * 1024)
#define ARCH_CACHE_L1_ICACHE_ASSOC      1
#define ARCH_CACHE_L1_ICACHE_LINESIZE   32

#define ARCH_CACHE_L1_DCACHE_SIZE       (16 * 1024)
#define ARCH_CACHE_L1_DCACHE_ASSOC      1
#define ARCH_CACHE_L1_DCACHE_LINESIZE   32

#define ARCH_CACHE_L2_CACHE_SIZE        0
#define ARCH_CACHE_L2_CACHE_ASSOC       0
#define ARCH_CACHE_L2_CACHE_LINESIZE    0

void arch_icache_inval_range(uintptr_t start, size_t count);
void arch_icache_sync_range(uintptr_t start, size_t count);

__depr("dcache_wback_sq is deprecated. Use sq_flush() from <dc/sq.h>")
static __always_inline void dcache_wback_sq(void *src) {
    __asm__ __volatile__("pref @%0\n"
                         : /* No outputs */
                         : "r" (src)
                         : "memory"
    );
}

static inline void arch_dcache_pref_line(const void *src) {
    __builtin_prefetch(src);
}

static inline void arch_dcache_alloc_line_with_value(void *src, uintptr_t value) {
    uintptr_t *ptr = (uintptr_t *)src;

    __asm__ ("movca.l r0, @%8\n\t"
             : "=m"(ptr[0]),
               "=m"(ptr[1]),
               "=m"(ptr[2]),
               "=m"(ptr[3]),
               "=m"(ptr[4]),
               "=m"(ptr[5]),
               "=m"(ptr[6]),
               "=m"(ptr[7])
             : "r" (ptr), "z"(value)
    );
}

static inline void arch_dcache_alloc_line(void *src) {
#if __GNUC__ <= 9
    /* Avoid ICE on GCC 9 */
    uint32_t r0 = 0;
#else
    register uint32_t r0 asm("r0");
#endif

    arch_dcache_alloc_line_with_value(src, r0);
}

static inline void arch_dcache_zero_alloc_line(void *src) {
    uint32_t *ptr = (uint32_t *)((uintptr_t)src & ~0x1f);

    arch_dcache_alloc_line_with_value(src, 0);

    ptr[1] = ptr[2] = ptr[3] = ptr[4] = ptr[5] = ptr[6] = ptr[7] = 0;
}

static inline void arch_dcache_inval_line(void *src) {
    uintptr_t *ptr = (uintptr_t *)src;

    __asm__ ("ocbi @%8\n\t"
             : "=m"(ptr[0]),
               "=m"(ptr[1]),
               "=m"(ptr[2]),
               "=m"(ptr[3]),
               "=m"(ptr[4]),
               "=m"(ptr[5]),
               "=m"(ptr[6]),
               "=m"(ptr[7])
             : "r" (ptr)
    );
}

static inline void arch_dcache_purge_line(void *src) {
    uintptr_t *ptr = (uintptr_t *)src;

    __asm__ ("ocbp @%8\n\t"
             : "=m"(ptr[0]),
               "=m"(ptr[1]),
               "=m"(ptr[2]),
               "=m"(ptr[3]),
               "=m"(ptr[4]),
               "=m"(ptr[5]),
               "=m"(ptr[6]),
               "=m"(ptr[7])
             : "r" (ptr)
    );
}

static inline void arch_dcache_wback_line(void *src) {
    uintptr_t *ptr = (uintptr_t *)src;

    __asm__ ("ocbwb @%8\n\t"
             : "=m"(ptr[0]),
               "=m"(ptr[1]),
               "=m"(ptr[2]),
               "=m"(ptr[3]),
               "=m"(ptr[4]),
               "=m"(ptr[5]),
               "=m"(ptr[6]),
               "=m"(ptr[7])
             : "r" (ptr)
    );
}

static inline void arch_dcache_inval_range(uintptr_t start, size_t count) {
    uintptr_t end = start + count;

    start &= ~0x1f;

    for(; start < end; start += 32)
        arch_dcache_inval_line((void *)start);
}

static inline void arch_dcache_wback_all(void) {
    unsigned int i;
    volatile uint32_t *dca = (volatile uint32_t *)0xf4000008;

    for (i = 0; i < 512; i++, dca += 8)
        *dca &= ~BIT(1); /* Zero out U bit */
}

static inline void arch_dcache_wback_range(uintptr_t start, size_t count) {
    uintptr_t end = start + count;

    if(count >= 65560) {
        /* Above this magic threshold, it's just faster to flush the whole cache. */
        arch_dcache_wback_all();
    }
    else {
        start &= ~0x1f;

        for(; start < end; start += 32)
            arch_dcache_wback_line((void *)start);
    }
}

static inline void arch_dcache_purge_all(void) {
    unsigned int i;

    if(__is_defined(__OPTIMIZE_SIZE__)) {
        volatile uint32_t *dca = (volatile uint32_t *)0xf4000008;

        for (i = 0; i < 512; i++, dca += 8)
            *dca = 0;
    }
    else {
        alignas(32) static char buffer[ARCH_CACHE_L1_DCACHE_SIZE];
        char *buf = buffer;

        for(i = 0; i < ARCH_CACHE_L1_DCACHE_SIZE / 32; i++) {
            arch_dcache_alloc_line(buf);
            arch_dcache_inval_line(buf);
            buf += 32;
        }
    }
}

static inline void arch_dcache_purge_range(uintptr_t start, size_t count) {
    uintptr_t end = start + count;

    if(count >= 39936) {
        /* Above this magic threshold, it's just faster to purge the whole cache. */
        arch_dcache_purge_all();
    }
    else {
        start &= ~0x1f;

        for(; start < end; start += 32)
            arch_dcache_purge_line((void *)start);
    }
}

__END_DECLS

#endif  /* __ARCH_CACHE_H */
