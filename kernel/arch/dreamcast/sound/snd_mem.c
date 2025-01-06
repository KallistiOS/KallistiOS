/* KallistiOS ##version##

   snd_mem.c
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2023 Ruslan Rostovtsev

 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/queue.h>
#include <dc/sound/sound.h>
#include <arch/spinlock.h>

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
    uint32  addr;

    /* The size of this block */
    size_t  size;

    /* Is this block in use? */
    int inuse;
} snd_block_t;

/* Our SPU RAM pool */
static int initted = 0;
static TAILQ_HEAD(snd_block_q, snd_block_str) pool = {0};
static spinlock_t snd_mem_mutex = SPINLOCK_INITIALIZER;


/* Reinitialize the pool with the given RAM base offset */
int snd_mem_init(uint32 reserve) {
    snd_block_t *blk;

    if(initted)
        snd_mem_shutdown();

    if(irq_inside_int()) {
        if(!spinlock_trylock(&snd_mem_mutex)) {
            errno = EAGAIN;
            return -1;
        }
    }
    else {
        spinlock_lock(&snd_mem_mutex);
    }

    // Make sure our base is 32-byte aligned
    reserve = (reserve + 0x1f) & ~0x1f;

    /* Make sure our tailq is initted */
    TAILQ_INIT(&pool);

    blk = (snd_block_t *)malloc(sizeof(snd_block_t));

    if(!blk) {
        spinlock_unlock(&snd_mem_mutex);
        errno = ENOMEM;
        return -1;
    }

    memset(blk, 0, sizeof(snd_block_t));
    blk->addr = reserve;
    blk->size = 2 * 1024 * 1024 - reserve;
    blk->inuse = 0;
    TAILQ_INSERT_HEAD(&pool, blk, qent);

#if SNDMEMDEBUG
    dbglog(DBG_DEBUG, "snd_mem_init: %d bytes available\n", blk->size);
#endif

    initted = 1;
    spinlock_unlock(&snd_mem_mutex);

    return 0;
}

/* Shut down the SPU allocator */
void snd_mem_shutdown(void) {
    snd_block_t *e, *n;

    if(!initted) return;

    if(irq_inside_int()) {
        if(!spinlock_trylock(&snd_mem_mutex)) {
            errno = EAGAIN;
            return;
        }
    }
    else {
        spinlock_lock(&snd_mem_mutex);
    }

    e = TAILQ_FIRST(&pool);

    while(e) {
        n = TAILQ_NEXT(e, qent);
#if SNDMEMDEBUG

        if(e->inuse)
            dbglog(DBG_DEBUG, "snd_mem_shutdown: in-use block at %08lx (size %d)\n", e->addr, e->size);
        else
            dbglog(DBG_DEBUG, "snd_mem_shutdown: unused block at %08lx (size %d)\n", e->addr, e->size);

#endif
        free(e);
        e = n;
    }

    initted = 0;
    spinlock_unlock(&snd_mem_mutex);
}

/* Allocate a chunk of SPU RAM; we will return an offset into SPU RAM. */
uint32 snd_mem_malloc(size_t size) {
    snd_block_t *e, *best = NULL;
    size_t best_size = 4 * 1024 * 1024;

    assert_msg(initted, "Use of snd_mem_malloc before snd_mem_init");

    if(size == 0)
        return 0;

    if(irq_inside_int()) {
        if(!spinlock_trylock(&snd_mem_mutex)) {
            errno = EAGAIN;
            return 0;
        }
    }
    else {
        spinlock_lock(&snd_mem_mutex);
    }

    // Make sure the size is a multiple of 32 bytes to maintain alignment
    size = (size + 0x1f) & ~0x1f;

    /* Look for a block */
    TAILQ_FOREACH(e, &pool, qent) {
        if(e->size >= size && e->size < best_size && !e->inuse) {
            best_size = e->size;
            best = e;
        }
    }

    if(best == NULL) {
        dbglog(DBG_ERROR, "snd_mem_malloc: no chunks big enough for alloc(%d)\n", size);
        spinlock_unlock(&snd_mem_mutex);
        return 0;
    }

    /* Is the block the exact size? */
    if(best->size == size) {
#if SNDMEMDEBUG
        dbglog(DBG_DEBUG, "snd_mem_malloc: allocating perfect-fit at %08lx for size %d\n", best->addr, best->size);
#endif
        best->inuse = 1;
        spinlock_unlock(&snd_mem_mutex);
        return best->addr;
    }

    /* Nope: break it up into two chunks */
    e = (snd_block_t*)malloc(sizeof(snd_block_t));

    if(e == NULL) {
        dbglog(DBG_ERROR, "snd_mem_malloc: not enough main memory to alloc(%d)\n", size);
        spinlock_unlock(&snd_mem_mutex);
        return 0;
    }

    memset(e, 0, sizeof(snd_block_t));
    e->addr = best->addr + size;
    e->size = best->size - size;
    e->inuse = 0;
    TAILQ_INSERT_AFTER(&pool, best, e, qent);

#if SNDMEMDEBUG
    dbglog(DBG_DEBUG, "snd_mem_malloc: allocating block %08lx for size %d, and leaving %d at %08lx\n",
           best->addr, size, e->size, e->addr);
#endif

    best->size = size;
    best->inuse = 1;

    spinlock_unlock(&snd_mem_mutex);
    return best->addr;
}

/* Free a chunk of SPU RAM; pointer is expected to be an offset into
   SPU RAM. */
void snd_mem_free(uint32 addr) {
    snd_block_t *e, *o;

    assert_msg(initted, "Use of snd_mem_free before snd_mem_init");

    if(addr == 0)
        return;

    if(irq_inside_int()) {
        if(!spinlock_trylock(&snd_mem_mutex)) {
            errno = EAGAIN;
            return;
        }
    }
    else {
        spinlock_lock(&snd_mem_mutex);
    }

    /* Look for the block */
    TAILQ_FOREACH(e, &pool, qent) {
        if(e->addr == addr)
            break;
    }

    if(!e) {
        dbglog(DBG_ERROR, "snd_mem_free: attempt to free non-existent block at %08lx\n", (uint32)e);
        spinlock_unlock(&snd_mem_mutex);
        return;
    }

    /* Set this block as unused */
    e->inuse = 0;

#if SNDMEMDEBUG
    dbglog(DBG_DEBUG, "snd_mem_free: freeing block at %08lx\n", e->addr);
#endif

    /* Can we coalesce with the block before us? */
    o = TAILQ_PREV(e, snd_block_q, qent);

    if(o && !o->inuse) {
#if SNDMEMDEBUG
        dbglog(DBG_DEBUG, "   coalescing with block at %08lx\n", o->addr);
#endif

        o->size += e->size;
        TAILQ_REMOVE(&pool, e, qent);
        free(e);
        e = o;
    }

    /* Can we coalesce with the block in front of us? */
    o = TAILQ_NEXT(e, qent);

    if(o && !o->inuse) {
#if SNDMEMDEBUG
        dbglog(DBG_DEBUG, "   coalescing with block at %08lx\n", o->addr);
#endif

        e->size += o->size;
        TAILQ_REMOVE(&pool, o, qent);
        free(o);
    }
    spinlock_unlock(&snd_mem_mutex);
}

uint32 snd_mem_available(void) {
    snd_block_t *e;
    size_t largest = 0;

    if(!initted)
        return 0;

    if(irq_inside_int()) {
        if(!spinlock_trylock(&snd_mem_mutex)) {
            errno = EAGAIN;
            return 0;
        }
    }
    else {
        spinlock_lock(&snd_mem_mutex);
    }

    TAILQ_FOREACH(e, &pool, qent) {
        if(e->size > largest)
            largest = e->size;
    }

    spinlock_unlock(&snd_mem_mutex);
    return (uint32)largest;
}
