/* KallistiOS ##version##

   arch/null/include/cache.h
   Copyright (C) 2026 Donald Haase
*/

/** \file    arch/cache.h
    \brief   Cache management functionality.
    \ingroup system_cache

    This file contains stubs to fake that the null arch has cache management.
*/

#ifndef __ARCH_CACHE_H
#define __ARCH_CACHE_H

#include <kos/cdefs.h>
__BEGIN_DECLS

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

static void arch_icache_inval_range(uintptr_t start, size_t count) {(void)start; (void)count;}
static void arch_icache_sync_range(uintptr_t start, size_t count) {(void)start; (void)count;}
static void arch_dcache_inval_range(uintptr_t start, size_t count) {(void)start; (void)count;}
static void arch_dcache_wback_range(uintptr_t start, size_t count) {(void)start; (void)count;}
static void arch_dcache_wback_all(void) {}
static void arch_dcache_purge_range(uintptr_t start, size_t count) {(void)start; (void)count;}
static void arch_dcache_purge_all(void) {}
static void arch_dcache_pref_line(const void *src) {(void)src;}
static void arch_dcache_alloc_line(void *src) {(void)src;}
static void arch_dcache_zero_alloc_line(void *src) {(void)src;}
static void arch_dcache_alloc_line_with_value(void *src, uintptr_t value) {(void)src; (void)value;}
static void arch_dcache_inval_line(void *src) {(void)src;}
static void arch_dcache_purge_line(void *src) {(void)src;}
static void arch_dcache_wback_line(void *src) {(void)src;}

__END_DECLS

#endif  /* __ARCH_CACHE_H */
