/* KallistiOS ##version##

   snd_mem.c
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2023, 2025 Ruslan Rostovtsev

 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/queue.h>
#include <dc/sound/sound.h>
#include <arch/arch.h>
#include <kos/dbglog.h>
#include <kos/mutex.h>

/*

This is a very simple allocator for SPU RAM. I decided not to go with dlmalloc
because of the massive number of changes it would require in the thing to
make it use the g2_* bus calls. This is just a lot more sane.

This still uses the same basic algorithm, though, it just doesn't bother
trying to be super efficient. This is based on the assumption that the
most common usage will be allocating or freeing a couple of very large
chunks every once in a while, not a ton of tiny chunks constantly (like
dlmalloc is optimized for).

The malloc algorithm used here is a basic "best fit" algorithm. We store a
linked list of available chunks of sound RAM in regular RAM and traverse it
to find the chunk that is the smallest while still large enough to fit the
block size we want. If there is any space left over, this chunk is broken
into two chunks, the first one occupied and the second one unoccupied.

The free algorithm is very lazy: it attempts to coalesce with neighbor
blocks if any of them are free. Otherwise it simply tags the block as free
in the hopes that a later free will coalesce with it.

This system is a bit more simplistic than the standard "buckets" system, but
I figure we don't need that sort of complexity in this code.

*/

#define SNDMEMDEBUG 0

/* A single block of SPU RAM */
typedef struct snd_block_str {
    /* Our queue entry */
    TAILQ_ENTRY(snd_block_str)  qent;

    /* The address of this block (offset from SPU RAM base) */
    uint32_t  addr;

    /* The size of this block */
    size_t  size;

    /* Is this block in use? */
    bool inuse;
} snd_block_t;

/* Our SPU RAM pool */
static bool initted = false;
static TAILQ_HEAD(snd_block_q, snd_block_str) pool = {0};
static mutex_t snd_mem_mutex = MUTEX_INITIALIZER;


/* Reinitialize the pool with the given RAM base offset */
int snd_mem_init(uint32_t reserve) {
    snd_block_t *blk;

    if(initted)
        snd_mem_shutdown();

    if(mutex_lock_irqsafe(&snd_mem_mutex)) {
        errno = EAGAIN;
        return -1;
    }

    // Make sure our base is 32-byte aligned
    reserve = __align_up(reserve, 32);

    /* Make sure our tailq is initted */
    TAILQ_INIT(&pool);

    blk = (snd_block_t *)malloc(sizeof(snd_block_t));

    if(!blk) {
        mutex_unlock(&snd_mem_mutex);
        errno = ENOMEM;
        return -1;
    }

    memset(blk, 0, sizeof(snd_block_t));
    blk->addr = reserve;

    if(hardware_sys_mode(NULL) == HW_TYPE_RETAIL)
        blk->size = 2 * 1024 * 1024 - reserve;
    else
        blk->size = 8 * 1024 * 1024 - reserve;

    blk->inuse = false;
    TAILQ_INSERT_HEAD(&pool, blk, qent);

    dbglog(DBG_SOURCE(SNDMEMDEBUG), "snd_mem_init: %d bytes available\n", blk->size);

    initted = true;
    mutex_unlock(&snd_mem_mutex);

    return 0;
}

/* Shut down the SPU allocator */
void snd_mem_shutdown(void) {
    snd_block_t *e, *n;

    if(!initted) return;

    if(mutex_lock_irqsafe(&snd_mem_mutex))
        return;

    TAILQ_FOREACH_SAFE(e, &pool, qent, n) {
        dbglog(DBG_SOURCE(SNDMEMDEBUG), "snd_mem_shutdown: %s block at %08lx (size %d)\n",
               e->inuse ? "in-use" : "unused", e->addr, e->size);

        TAILQ_REMOVE(&pool, e, qent);
        free(e);
    }

    initted = false;
    mutex_unlock(&snd_mem_mutex);
}

/* Allocate a chunk of SPU RAM; we will return an offset into SPU RAM. */
uint32_t snd_mem_malloc(size_t size) {
    snd_block_t *e, *best = NULL;
    size_t best_size = SIZE_MAX;

    assert_msg(initted, "Use of snd_mem_malloc before snd_mem_init");

    if(size == 0)
        return 0;

    if(mutex_lock_irqsafe(&snd_mem_mutex)) {
        errno = EAGAIN;
        return 0;
    }

    // Make sure the size is a multiple of 32 bytes to maintain alignment
    size = __align_up(size, 32);

    /* Look for a block */
    TAILQ_FOREACH(e, &pool, qent) {
        if(e->size >= size && e->size < best_size && !e->inuse) {
            best_size = e->size;
            best = e;
        }
    }

    if(best == NULL) {
        dbglog(DBG_ERROR, "snd_mem_malloc: no chunks big enough for alloc(%d)\n", size);
        mutex_unlock(&snd_mem_mutex);
        return 0;
    }

    /* Is the block the exact size? */
    if(best->size == size) {
        dbglog(DBG_SOURCE(SNDMEMDEBUG), "snd_mem_malloc: allocating perfect-fit at %08lx for size %d\n",
               best->addr, best->size);

        best->inuse = true;
        mutex_unlock(&snd_mem_mutex);
        return best->addr;
    }

    /* Nope: break it up into two chunks */
    e = (snd_block_t*)malloc(sizeof(snd_block_t));

    if(e == NULL) {
        dbglog(DBG_ERROR, "snd_mem_malloc: not enough main memory to alloc(%d)\n", size);
        mutex_unlock(&snd_mem_mutex);
        return 0;
    }

    memset(e, 0, sizeof(snd_block_t));
    e->addr = best->addr + size;
    e->size = best->size - size;
    e->inuse = false;
    TAILQ_INSERT_AFTER(&pool, best, e, qent);

    dbglog(DBG_SOURCE(SNDMEMDEBUG), "snd_mem_malloc: allocating block %08lx for size %d, and leaving %d at %08lx\n",
               best->addr, size, e->size, e->addr);

    best->size = size;
    best->inuse = true;

    mutex_unlock(&snd_mem_mutex);
    return best->addr;
}

/* Free a chunk of SPU RAM; pointer is expected to be an offset into
   SPU RAM. */
void snd_mem_free(uint32_t addr) {
    snd_block_t *e, *o;

    assert_msg(initted, "Use of snd_mem_free before snd_mem_init");

    if(addr == 0)
        return;

    if(mutex_lock_irqsafe(&snd_mem_mutex))
        return;

    /* Look for the block */
    TAILQ_FOREACH(e, &pool, qent) {
        if(e->addr == addr)
            break;
    }

    if(!e) {
        dbglog(DBG_ERROR, "snd_mem_free: attempt to free non-existent block at %08lx\n", (uint32_t)e);
        mutex_unlock(&snd_mem_mutex);
        return;
    }

    /* Set this block as unused */
    e->inuse = false;

    dbglog(DBG_SOURCE(SNDMEMDEBUG), "snd_mem_free: freeing block at %08lx\n", e->addr);

    /* Can we coalesce with the block before us? */
    o = TAILQ_PREV(e, snd_block_q, qent);

    if(o && !o->inuse) {
        dbglog(DBG_SOURCE(SNDMEMDEBUG), "   coalescing with block at %08lx\n", o->addr);

        o->size += e->size;
        TAILQ_REMOVE(&pool, e, qent);
        free(e);
        e = o;
    }

    /* Can we coalesce with the block in front of us? */
    o = TAILQ_NEXT(e, qent);

    if(o && !o->inuse) {
        dbglog(DBG_SOURCE(SNDMEMDEBUG), "   coalescing with block at %08lx\n", o->addr);

        e->size += o->size;
        TAILQ_REMOVE(&pool, o, qent);
        free(o);
    }
    mutex_unlock(&snd_mem_mutex);
}

uint32_t snd_mem_available(void) {
    snd_block_t *e;
    size_t largest = 0;

    if(!initted)
        return 0;

    if(mutex_lock_irqsafe(&snd_mem_mutex)) {
        errno = EAGAIN;
        return 0;
    }

    TAILQ_FOREACH(e, &pool, qent) {
        if(e->size > largest)
            largest = e->size;
    }

    mutex_unlock(&snd_mem_mutex);
    return (uint32_t)largest;
}
