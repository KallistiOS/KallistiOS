/* KallistiOS ##version##

   arch/dreamcast/include/cache.h
   Copyright (C) 2001 Megan Potter
   Copyright (C) 2014, 2016, 2023 Ruslan Rostovtsev
   Copyright (C) 2023 Andy Barajas
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
*/

#ifndef __ARCH_CACHE_H
#define __ARCH_CACHE_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <arch/types.h>

/** \defgroup system_cache Cache
    \brief                 Driver and API for managing the SH4's cache
    \ingroup               system

    @{
*/

/** \brief  SH4 cache block size.

    The size of a cache block.
*/
#define CPU_CACHE_BLOCK_SIZE 32

/** \brief  Flush the instruction cache.

    This function flushes a range of the instruction cache.

    \param  start           The physical address to begin flushing at.
    \param  count           The number of bytes to flush.
*/
void icache_flush_range(uintptr_t start, size_t count);

/** \brief  Invalidate the data/operand cache.

    This function invalidates a range of the data/operand cache. If you care
    about the contents of the cache that have not been written back yet, use
    dcache_flush_range() before using this function.

    \param  start           The physical address to begin invalidating at.
    \param  count           The number of bytes to invalidate.
*/
void dcache_inval_range(uintptr_t start, size_t count);

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
void dcache_flush_range(uintptr_t start, size_t count);

/** \brief  Flush all the data/operand cache.

    This function flushes all the data/operand cache, forcing a write-
    back on all of the cache blocks that are marked as dirty.

    \note
    dcache_flush_range() is faster than dcache_flush_all() if the count
    param is 66560 or less.
*/
void dcache_flush_all(void);

/** \brief  Purge the data/operand cache.

    This function flushes a range of the data/operand cache, forcing a write-
    back and then invalidates all of the data in the specified range.

    \param  start           The physical address to begin purging at.
    \param  count           The number of bytes to purge.
*/
void dcache_purge_range(uintptr_t start, size_t count);

/** \brief  Purge all the data/operand cache.

    This function flushes the entire data/operand cache, ensuring that all 
    cache blocks marked as dirty are written back to memory and all cache 
    entries are invalidated. It does not require an additional buffer and is 
    preferred when memory resources are constrained.

    \note
    dcache_purge_range() is faster than dcache_purge_all() if the count
    param is 39936 or less.
*/
void dcache_purge_all(void);

/** \brief  Purge all the data/operand cache with buffer.

    This function performs a purge of all data/operand cache blocks by 
    utilizing an external buffer to speed up the write-back and invalidation 
    process. It is always faster than dcache_purge_all() and is recommended 
    where maximum speed is required.

    \note While this function offers a superior purge rate, it does require
    the use of a temporary buffer. So use this function if you have an extra 
    8/16 kb of memory laying around that you can utilize for no other purpose 
    than for this function.

    \param  start           The physical address for temporary buffer (32-byte 
                            aligned)
    \param  count           The size of the temporary buffer, which can be 
                            either 8 KB or 16 KB, depending on cache 
                            configuration - 8 KB buffer with OCRAM enabled, 
                            otherwise 16 KB.

*/
void dcache_purge_all_with_buffer(uintptr_t start, size_t count);

/** \brief  Prefetch one block to the data/operand cache.

    This function prefetch a block of the data/operand cache.

    \param  src             The physical address to prefetch.
*/
static __always_inline void dcache_pref_block(const void *src) {
    __asm__ __volatile__("pref @%0\n"
                         : /* No outputs */
                         : "r" (src)
                         : /* No clobbers */
    );
}

/** \brief  Write-back Store Queue buffer to external memory

    This function initiates write-back for one Store Queue.

    \param  ptr             The SQ mapped address to write-back.
*/
#define dcache_wback_sq(ptr) dcache_pref_block(ptr)

/** \brief  Allocate one block of the data/operand cache.

    This function allocate a block of the data/operand cache.

    \param  src             The address to allocate (32-byte aligned)
    \param  value           The value written to first 4-byte.
*/
static __always_inline void dcache_alloc_block(void *src, uint32_t value) {
    uint32_t *src32 = (uint32_t *)src;

    __asm__ ("movca.l r0, @%8\n\t"
             : "=m"(src32[0]),
               "=m"(src32[1]),
               "=m"(src32[2]),
               "=m"(src32[3]),
               "=m"(src32[4]),
               "=m"(src32[5]),
               "=m"(src32[6]),
               "=m"(src32[7])
             : "r" (src32), "z" (value)
    );
}

/** @} */

__END_DECLS

#endif  /* __ARCH_CACHE_H */
