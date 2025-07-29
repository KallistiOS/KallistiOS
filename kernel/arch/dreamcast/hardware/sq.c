/* KallistiOS ##version##

   kernel/arch/dreamcast/hardware/sq.c
   Copyright (C) 2001 Andrew Kieschnick
   Copyright (C) 2023 Falco Girgis
   Copyright (C) 2023 Andy Barajas
   Copyright (C) 2023 Ruslan Rostovtsev
   Copyright (C) 2024 Donald Haase
*/

#include <assert.h>

#include <arch/cache.h>
#include <arch/mmu.h>
#include <dc/sq.h>
#include <kos/dbglog.h>
#include <kos/mutex.h>


/*
    Functions to clear, copy, and set memory using the sh4 store queues

    Based on code by Marcus Comstedt, TapamN, and Moop
*/

/** \brief   Store Queue 0 access register
    \ingroup store_queues
*/
#define QACR0 (*(volatile uint32_t *)(void *)0xff000038)

/** \brief   Store Queue 1 access register
    \ingroup store_queues
*/
#define QACR1 (*(volatile uint32_t *)(void *)0xff00003c)

/** \brief   Shift and filter bits needed for the QACR registers
    \ingroup store_queues
*/
#define QACR_EXTERN_BITS(dest) ((((uintptr_t)(dest)) >> 24) & 0x1c)

/** \brief   Set Store Queue QACR* registers from EXTERN_BITS
    \ingroup store_queues
*/
#define SET_QACR_REGS_INNER(dest0, dest1) \
    do { \
        QACR0 = (dest0); \
        QACR1 = (dest1); \
    } while(0)

/** \brief   Set Store Queue QACR* registers
    \ingroup store_queues
*/
#define SET_QACR_REGS(dest0, dest1) \
    SET_QACR_REGS_INNER(QACR_EXTERN_BITS(dest0), QACR_EXTERN_BITS(dest1))

static mutex_t sq_mutex = RECURSIVE_MUTEX_INITIALIZER;

typedef struct sq_state {
    uint32_t dest;
} sq_state_t;

#ifndef SQ_STATE_CACHE_SIZE
#define SQ_STATE_CACHE_SIZE 8
#endif

static sq_state_t sq_state_cache[SQ_STATE_CACHE_SIZE] = {0};

uint32_t *sq_lock(void *dest) {
    sq_state_t *new_state;
    bool with_mmu;
    uint32_t mask;

    mutex_lock(&sq_mutex);

    assert_msg(sq_mutex.count < SQ_STATE_CACHE_SIZE, "You've overrun the SQ_STATE_CACHE.");

    new_state = &sq_state_cache[sq_mutex.count - 1];

    new_state->dest = (uint32_t)dest;

    with_mmu = mmu_enabled();
    mask = with_mmu ? 0x000fffe0 : 0x03ffffe0;

    if(with_mmu)
        mmu_set_sq_addr(dest);
    else
        SET_QACR_REGS(dest, dest);

    return (uint32_t *)(MEM_AREA_SQ_BASE | ((uintptr_t)dest & mask));
}

void sq_unlock(void) {
    sq_state_t *tmp_state;
    bool with_mmu;

    if(sq_mutex.count == 0) {
        dbglog(DBG_WARNING, "sq_unlock: Called without any lock\n");
        return;
    }

    tmp_state = &sq_state_cache[sq_mutex.count - 1];

    /* If we aren't the last entry, set the regs back where they belong */
    if(sq_mutex.count - 1) {
        tmp_state = &sq_state_cache[sq_mutex.count - 2];
        with_mmu = mmu_enabled();

        if(with_mmu)
            mmu_set_sq_addr((void *)tmp_state->dest);
        else
            SET_QACR_REGS(tmp_state->dest, tmp_state->dest);
    }

    mutex_unlock(&sq_mutex);
}

void sq_wait(void) {
    /* Wait for both store queues to complete */
    uint32_t *d = (uint32_t *)MEM_AREA_SQ_BASE;
    d[0] = d[8] = 0;
}

/* Copies n bytes from src to dest, dest must be 32-byte aligned */
__no_inline void *sq_cpy(void *dest, const void *src, size_t n) {
    const uint32_t *s = src;
    void *curr_dest = dest;
    uint32_t *d;
    size_t nb;

    /* Fill/write queues as many times necessary */
    n >>= 5;

    while(n > 0) {
        /* Transfer maximum 1 MiB at once. This is because when using the
         * MMU the SQ area is 2 MiB, and the destination address may
         * not be on a page boundary. */
        nb = n > 0x8000 ? 0x8000 : n;

        d = sq_lock(curr_dest);

        curr_dest += nb * 32;
        n -= nb;

        /* If src is not 8-byte aligned, slow path */
        if(!__is_aligned(src, 8)) {
            while(nb--) {
                dcache_pref_block(s + 8); /* Prefetch 32 bytes for next loop */
                d[0] = *(s++);
                d[1] = *(s++);
                d[2] = *(s++);
                d[3] = *(s++);
                d[4] = *(s++);
                d[5] = *(s++);
                d[6] = *(s++);
                d[7] = *(s++);
                sq_flush(d);
                d += 8;
            }
        } else { /* If src is 8-byte aligned, fast path */
            sq_fast_cpy(d, s, nb);
            s += nb * 32;
        }

        sq_unlock();
    }

    return dest;
}

/* Fills n bytes at dest with byte c, dest must be 32-byte aligned */
void *sq_set(void *dest, uint32_t c, size_t n) {
    /* Duplicate low 8-bits of c into high 24-bits */
    c = c & 0xff;
    c = (c << 24) | (c << 16) | (c << 8) | c;

    return sq_set32(dest, c, n);
}

/* Fills n bytes at dest with short c, dest must be 32-byte aligned */
void *sq_set16(void *dest, uint32_t c, size_t n) {
    /* Duplicate low 16-bits of c into high 16-bits */
    c = c & 0xffff;
    c = (c << 16) | c;

    return sq_set32(dest, c, n);
}

/* Fills n bytes at dest with int c, dest must be 32-byte aligned */
void *sq_set32(void *dest, uint32_t c, size_t n) {
    void *curr_dest = dest;
    uint32_t *d;
    size_t nb;

    /* Write them as many times necessary */
    n >>= 5;

    while(n > 0) {
        /* Transfer maximum 1 MiB at once. This is because when using the
         * MMU the SQ area is 2 MiB, and the destination address may
         * not be on a page boundary. */
        nb = n > 0x8000 ? 0x8000 : n;

        d = sq_lock(curr_dest);

        curr_dest += nb * 32;
        n -= nb;

        while(nb--) {
            /* Fill both store queues with c */
            d[0] = d[1] = d[2] = d[3] = d[4] = d[5] = d[6] = d[7] = c;
            sq_flush(d);
            d += 8;
        }

        sq_unlock();
    }

    return dest;
}

/* Clears n bytes at dest, dest must be 32-byte aligned */
void sq_clr(void *dest, size_t n) {
    sq_set32(dest, 0, n);
}
