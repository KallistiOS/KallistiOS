/* KallistiOS ##version##

   arch/dreamcast/include/cache.h
   Copyright (C) 2001 Megan Potter
   Copyright (C) 2014, 2016, 2023 Ruslan Rostovtsev

*/

/** \file   arch/cache.h
    \brief  Cache management functionality.

    This file contains definitions for functions that manage the cache in the
    Dreamcast, including functions to flush, invalidate, purge, prefetch and
    allocate the caches.

    \author Megan Potter
    \author Ruslan Rostovtsev
*/

#ifndef __ARCH_CACHE_H
#define __ARCH_CACHE_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/** \brief  SH4 cache block size.

    The physical address will be aligned to this size in all
    functions except dcache_alloc_write.
*/
#define CPU_CACHE_BLOCK_SIZE 32

/** \brief  Flush the instruction cache.

    This function flushes a range of the instruction cache.

    \param  start           The physical address to begin flushing at.
    \param  count           The number of bytes to flush.
*/
void icache_flush_range(uint32 start, uint32 count);

/** \brief  Invalidate the data/operand cache.

    This function invalidates a range of the data/operand cache. If you care
    about the contents of the cache that have not been written back yet, use
    dcache_flush_range() before using this function.

    \param  start           The physical address to begin invalidating at.
    \param  count           The number of bytes to invalidate.
*/
void dcache_inval_range(uint32 start, uint32 count);

/** \brief  Flush the data/operand cache.

    This function flushes a range of the data/operand cache, forcing a write-
    back on all of the data in the specified range. This does not invalidate the
    cache in the process (meaning the blocks will still be in the cache, just
    not marked as dirty after this has completed). If you wish to invalidate the
    cache as well, call dcache_inval_range() after calling this function or
    use dcache_purge_range() instead of dcache_flush_range().

    \param  start           The physical address to begin flushing at.
    \param  count           The number of bytes to flush.
*/
void dcache_flush_range(uint32 start, uint32 count);

/** \brief  Purge the data/operand cache.

    This function flushes a range of the data/operand cache, forcing a write-
    back and invalidate on all of the data in the specified range.

    \param  start           The physical address to begin purging at.
    \param  count           The number of bytes to purge.
*/
void dcache_purge_range(uint32 start, uint32 count);

/** \brief  Prefetch memory to the data/operand cache.

    This function prefetch a range of the data/operand cache.

    \param  start           The physical address to begin prefetching at.
    \param  count           The number of bytes to prefetch.
    \return                 The physical address aligned to cache block size.
*/
void *dcache_pref_range(uint32 start, uint32 count);

/** \brief  Prefetch one block to the data/operand cache.

    This function prefetch a range of the data/operand cache.

    \param  src             The buffer to prefetch.
    \return                 The buffer aligned to cache block size.
*/
static __always_inline void *dcache_pref_block(const void *src) {
    uint32 __cache_aligned = ((uint32)src) & ~(CPU_CACHE_BLOCK_SIZE - 1);
    __asm__ __volatile__("pref @%[ptr]\n"
                         :
                         : [ptr] "r" (__cache_aligned)
                         :
    );

    return (void *)__cache_aligned;
}

__END_DECLS

#endif  /* __ARCH_CACHE_H */
