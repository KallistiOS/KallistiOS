/* KallistiOS ##version##

   include/kos/cache.h
   Copyright (C) 2001 Megan Potter
   Copyright (C) 2014, 2016, 2023 Ruslan Rostovtsev
   Copyright (C) 2023 Andy Barajas
   Copyright (C) 2025 Eric Fradella
   Copyright (C) 2026 Falco Girgis
*/

/** \file    include/kos/cache.h
    \brief   Cache management functionality.
    \ingroup system_cache

    This file contains definitions for functions that manage the caches,
    including functions to flush, invalidate, purge, prefetch and allocate
    individual cache lines or address ranges.

    \author Megan Potter
    \author Ruslan Rostovtsev
    \author Andy Barajas
    \author Falco Girgis
*/

#ifndef __KOS_CACHE_H
#define __KOS_CACHE_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <arch/cache.h>

#include <stdint.h>

/** \defgroup system_cache Cache
    \brief                 Driver and API for managing the SH4's cache
    \ingroup               system

    @{
*/

/** \brief  Level 1 instruction cache size.

    The capacity of the L1 instruction cache in bytes.
*/
#define CACHE_L1_ICACHE_SIZE \
    ARCH_CACHE_L1_ICACHE_SIZE

/** \brief  Level 1 instruction cache associativity.

    Number of ways in the L1 instruction cache.
*/
#define CACHE_L1_ICACHE_ASSOC \
    ARCH_CACHE_L1_ICACHE_ASSOC

/** \brief  L1 instruction cache line size.

    The size of each cache line in the L1 instruction cache.
*/
#define CACHE_L1_ICACHE_LINESIZE \
    ARCH_CACHE_L1_ICACHE_LINESIZE

/** \brief  Level 1 data cache size.

    The capacity of the L1 data cache in bytes.
*/
#define CACHE_L1_DCACHE_SIZE \
    ARCH_CACHE_L1_DCACHE_SIZE

/** \brief  Level 1 data cache associativity.

    Number of ways in the L1 data cache.
*/
#define CACHE_L1_DCACHE_ASSOC \
    ARCH_CACHE_L1_DCACHE_ASSOC

/** \brief  L1 data cache line size.

    The size of each cache line in the L1 data cache.
*/
#define CACHE_L1_DCACHE_LINESIZE \
    ARCH_CACHE_L1_DCACHE_LINESIZE

/** \brief  Level 2 cache size.

    The capacity of the L2 cache in bytes.
*/
#define CACHE_L2_CACHE_SIZE \
    ARCH_CACHE_L2_CACHE_SIZE

/** \brief  Level 2 cache associativity.

    Number of ways in the L2 cache.
*/
#define CACHE_L2_CACHE_ASSOC \
    ARCH_CACHE_L2_CACHE_ASSOC

/** \brief  Level 2 cache line size.

    The size of each cache line in the L2 cache.
*/
#define CACHE_L2_CACHE_LINESIZE \
    ARCH_CACHE_L2_CACHE_LINESIZE

/** \brief Invalidate the instruction cache.

    This instruction invalidates a range of the instruction cache.

    \param  start           The physical address to begin invalidation at.
    \param  count           The number of bytes to invalidate.
*/
static inline void icache_inval_range(uintptr_t start, size_t count) {
    arch_icache_inval_range(start, count);
}

/** \brief Synchronize the instruction cache.

    This function ensures that the instruction cache is synchronized with the
    data/operand cache. It is functionally  the same as calling
    dcache_flush_range() followed by icache_inval_range(), but may be
    implemented in a more optimized way.

    \param  start           The physical address to begin invalidation at.
    \param  count           The number of bytes to invalidate.
*/
static inline void icache_sync_range(uintptr_t start, size_t count) {
    arch_icache_sync_range(start, count);
}

/** \brief  Invalidate the data/operand cache.

    This function invalidates a range of the data/operand cache. If you care
    about the contents of the cache that have not been written back yet, use
    dcache_flush_range() before using this function.

    \param  start           The physical address to begin invalidating at.
    \param  count           The number of bytes to invalidate.
*/
static inline void dcache_inval_range(uintptr_t start, size_t count) {
    arch_dcache_inval_range(start, count);
}

/** \brief  Flush the data/operand cache.

    This function flushes a range of the data/operand cache, forcing a write-
    back on all of the data in the specified range. This does not invalidate
    the cache in the process (meaning the blocks will still be in the cache,
    just not marked as dirty after this has completed). If you wish to
    invalidate the cache as well, call dcache_inval_range() after calling this
    function or use dcache_purge_range() instead of dcache_flush_range().

    \param  start           The physical address to begin flushing at.
    \param  count           The number of bytes to flush.
*/
static inline void dcache_flush_range(uintptr_t start, size_t count) {
    arch_dcache_flush_range(start, count);
}

/** \brief  Flush all the data/operand cache.

    This function flushes all the data/operand cache, forcing a write-
    back on all of the cache blocks that are marked as dirty.

    \note
    dcache_flush_range() is faster than dcache_flush_all() if the count
    param is 66560 or less.
*/
static inline void dcache_flush_all(void) {
    arch_dcache_flush_all();
}

/** \brief  Purge the data/operand cache.

    This function flushes a range of the data/operand cache, forcing a write-
    back and then invalidates all of the data in the specified range.

    \param  start           The physical address to begin purging at.
    \param  count           The number of bytes to purge.
*/
static inline void dcache_purge_range(uintptr_t start, size_t count) {
    arch_dcache_purge_range(start, count);
}

/** \brief  Purge all the data/operand cache.

    This function flushes the entire data/operand cache, ensuring that all
    cache blocks marked as dirty are written back to memory and all cache
    entries are invalidated. It does not require an additional buffer and is
    preferred when memory resources are constrained.

    \note
    dcache_purge_range() is faster than dcache_purge_all() if the count
    param is 39936 or less.
*/
static inline void dcache_purge_all(void) {
    arch_dcache_purge_all();
}

/** \brief  Prefetch one block to the data/operand cache.

    This function prefetch a block of the data/operand cache.

    \param  src             The physical address to prefetch.
*/
static inline void dcache_pref_line(const void *src) {
    arch_dcache_pref_line(src);
}

/** \brief  Allocate one cache line of the data/operand cache.

    This function allocates a cache line of the data/operand cache.

    \param  src             The address to allocate (32-byte aligned)
    \param  value           The value written to the beginning of the cache line.
*/
static inline void dcache_alloc_line(void *src, uintptr_t value) {
    arch_dcache_alloc_line(src, value);
}

/** \brief  Invalidate one cache line of the data/operand cache.

    This function invalidates a cache line of the data/operand cache.
    The data inside the cache line is not written back to memory, and
    the cache line is marked as free.

    \param  src             The address to invalidate
*/
static inline void dcache_inval_line(void *src) {
    arch_dcache_inval_line(src);
}

/** \brief  Purge one cache line of the data/operand cache.

    This function purges a cache line of the data/operand cache.
    If the cache line is dirty, the data is written back to memory, and
    then the cache line is marked as free.

    \param  src             The address to purge
*/
static inline void dcache_purge_line(void *src) {
    arch_dcache_purge_line(src);
}

/** \brief  Flush one cache line of the data/operand cache.

    This function flushes a cache line of the data/operand cache.
    If the cache line is dirty, the data is written back to memory.
    The cache line is not invalidated.

    \param  src             The address to flush
*/
static inline void dcache_flush_line(void *src) {
    arch_dcache_flush_line(src);
}

/** \cond */
__depr("icache_flush_range() has been renamed to icache_sync_range()")
static inline void icache_flush_range(uintptr_t start, size_t count) {
    icache_sync_range(start, count);
}

__depr("dcache_pref_block() has been renamed to dcache_pref_line()")
static inline void dcache_pref_block(const void *src) {
    dcache_pref_line(src);
}

__depr("dcache_alloc_block() has been renamed to dcache_alloc_line()")
static inline void dcache_alloc_block(void *src, uint32_t value) {
    dcache_alloc_line(src, value);
}
/** \endcond */

/** @} */

__END_DECLS

#endif  /* __KOS_CACHE_H */
