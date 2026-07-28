/* KallistiOS ##version##

   arch/xbox/include/arch/cache.h
   Copyright (C) 2026 The KallistiOS Contributors
*/

/** \file    arch/cache.h
    \brief   Original Xbox CPU cache management.
    \ingroup system_cache

    The retail Xbox CPU reports, through CPUID, separate 16 KiB L1 instruction
    and data caches and a unified 128 KiB L2 cache. All use 32-byte lines.

    This Coppermine-derived CPU does not report CLFLUSH support. Until KOS owns
    the Xbox memory types and has device-specific DMA coherency policies, data
    cache maintenance therefore uses the privileged WBINVD instruction. This
    is deliberately conservative and operates on the whole cache hierarchy.
*/

/* Keep this include above the macro guard; <kos/cache.h> and this header form
   the same controlled include cycle used by the other KOS architectures. */
#include <kos/cache.h>

#ifndef __ARCH_CACHE_H
#define __ARCH_CACHE_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stddef.h>
#include <stdint.h>

#define ARCH_CACHE_L1_ICACHE_SIZE       (16U * 1024U)
#define ARCH_CACHE_L1_ICACHE_ASSOC      4U
#define ARCH_CACHE_L1_ICACHE_LINESIZE   32U

#define ARCH_CACHE_L1_DCACHE_SIZE       (16U * 1024U)
#define ARCH_CACHE_L1_DCACHE_ASSOC      4U
#define ARCH_CACHE_L1_DCACHE_LINESIZE   32U

#define ARCH_CACHE_L2_CACHE_SIZE        (128U * 1024U)
#define ARCH_CACHE_L2_CACHE_ASSOC       4U
#define ARCH_CACHE_L2_CACHE_LINESIZE    32U

/** Serialize instruction execution after code has been modified.

    IA-32 has coherent instruction and data caches. On this single-processor
    system no physical instruction-cache invalidation is required, but a
    serializing instruction is required before executing newly written code.
    CPUID is supported by the Xbox CPU and provides that serialization.
*/
static __always_inline void arch_cache_serialize(void) {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;

    eax = 0;
    __asm__ volatile("cpuid"
                     : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     :
                     : "memory");
}

/** Write back and invalidate the complete CPU cache hierarchy.

    Xbox titles execute in ring 0, which the hardware probe confirmed by
    successfully reading CR0 and CR4. WBINVD is therefore available here.
*/
static __always_inline void arch_cache_wbinvd(void) {
    __asm__ volatile("wbinvd" : : : "memory");
}

static inline void arch_icache_inval_range(uintptr_t start, size_t count) {
    (void)start;
    (void)count;
    arch_cache_serialize();
}

static inline void arch_icache_sync_range(uintptr_t start, size_t count) {
    (void)start;
    (void)count;

    /* Stores are globally visible before CPUID completes, and CPUID discards
       prefetched/decode state before control reaches newly written code. */
    arch_cache_serialize();
}

static inline void arch_dcache_pref_line(const void *src) {
    __builtin_prefetch(src, 0, 3);
}

static inline void arch_dcache_alloc_line(void *src) {
    __builtin_prefetch(src, 1, 3);
}

static inline void arch_dcache_alloc_line_with_value(void *src,
                                                      uintptr_t value) {
    __builtin_prefetch(src, 1, 3);
    *(uintptr_t *)src = value;
}

static inline void arch_dcache_zero_alloc_line(void *src) {
    uint32_t *line = (uint32_t *)((uintptr_t)src &
                                  ~(uintptr_t)(ARCH_CACHE_L1_DCACHE_LINESIZE -
                                               1U));
    size_t i;

    for(i = 0; i < ARCH_CACHE_L1_DCACHE_LINESIZE / sizeof(*line); ++i)
        line[i] = 0;
}

static inline void arch_dcache_inval_line(void *src) {
    (void)src;
    arch_cache_wbinvd();
}

static inline void arch_dcache_purge_line(void *src) {
    (void)src;
    arch_cache_wbinvd();
}

static inline void arch_dcache_wback_line(void *src) {
    (void)src;
    arch_cache_wbinvd();
}

static inline void arch_dcache_inval_range(uintptr_t start, size_t count) {
    if(count) {
        (void)start;
        arch_cache_wbinvd();
    }
}

static inline void arch_dcache_wback_range(uintptr_t start, size_t count) {
    if(count) {
        (void)start;
        arch_cache_wbinvd();
    }
}

static inline void arch_dcache_purge_range(uintptr_t start, size_t count) {
    if(count) {
        (void)start;
        arch_cache_wbinvd();
    }
}

static inline void arch_dcache_wback_all(void) {
    arch_cache_wbinvd();
}

static inline void arch_dcache_purge_all(void) {
    arch_cache_wbinvd();
}

__END_DECLS

#endif /* __ARCH_CACHE_H */
