/* KallistiOS ##version##

   kernel/arch/dreamcast/hardware/sq.c
   Copyright (C) 2001 Andrew Kieschnick
   Copyright (C) 2023 Falco Girgis
   Copyright (C) 2023 Andy Barajas
   Copyright (C) 2023 Ruslan Rostovtsev
*/

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

/** \brief   Set Store Queue QACR* registers
    \ingroup store_queues
*/
#define SET_QACR_REGS(dest0, dest1) \
    do { \
        QACR0 = (((uintptr_t)(dest0)) >> 24) & 0x1c; \
        QACR1 = (((uintptr_t)(dest1)) >> 24) & 0x1c; \
    } while(0)

static mutex_t sq_mutex = MUTEX_INITIALIZER;

static mmu_token_t mmu_token;

void sq_lock(void *dest) {
    mutex_lock(&sq_mutex);

    /* Disable MMU, because SQs work differently when it's enabled, and we
     * don't support it. */
    mmu_token = mmu_disable();

    SET_QACR_REGS(dest, dest);
}

void sq_unlock(void) {
    mmu_restore(mmu_token);
    mutex_unlock(&sq_mutex);
}

void sq_wait(void) {
    /* Wait for both store queues to complete */
    uint32_t *d = (uint32_t *)MEM_AREA_SQ_BASE;
    d[0] = d[8] = 0;
}

/* Copies n bytes from src to dest, dest must be 32-byte aligned */
__attribute__((noinline)) void *sq_cpy(void *dest, const void *src, size_t n) {
    uint32_t *d = SQ_MASK_DEST(dest);
    const uint32_t *s = src;

    /* Fill/write queues as many times necessary */
    n >>= 5;

    /* Exit early if we dont have enough data to copy */
    if(n == 0)
        return dest;

    sq_lock(dest);

    /* If src is not 8-byte aligned, slow path */
    if ((uintptr_t)src & 7) {
        while(n--) {
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
        sq_fast_cpy(d, s, n);
    }

    sq_unlock();
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
    uint32_t *d = SQ_MASK_DEST(dest);

    /* Write them as many times necessary */
    n >>= 5;

    /* Exit early if we dont have enough data to set */
    if(n == 0)
        return dest;

    sq_lock(dest);

    while(n--) {
        /* Fill both store queues with c */
        d[0] = d[1] = d[2] = d[3] = d[4] = d[5] = d[6] = d[7] = c;
        sq_flush(d);
        d += 8;
    }

    sq_unlock();
    return dest;
}

/* Clears n bytes at dest, dest must be 32-byte aligned */
void sq_clr(void *dest, size_t n) {
    sq_set32(dest, 0, n);
}
