/* KallistiOS ##version##

   kernel/arch/dreamcast/hardware/sq.c
   Copyright (C) 2001 Andrew Kieschnick
*/

#include <arch/memory.h>
#include <dc/sq.h>

/*
    Functions to clear, copy, and set memory using the sh4 store queues

    Based on code by Marcus Comstedt (store_q_clear from tatest)
*/

/* clears n bytes at dest, dest must be 32-byte aligned */
void sq_clr(void *dest, int n) {
    unsigned int *d = (unsigned int *)(void *)
                      (MEM_AREA_SQ_BASE | (((unsigned long)dest) & 0x03ffffe0));

    /* Set store queue memory area as desired */
    QACR0 = ((((unsigned int)dest) >> 26) << 2) & 0x1c;
    QACR1 = ((((unsigned int)dest) >> 26) << 2) & 0x1c;

    /* Fill both store queues with zeroes */
    d[0] = d[1] = d[2] = d[3] = d[4] = d[5] = d[6] = d[7] =
                                           d[8] = d[9] = d[10] = d[11] = d[12] = d[13] = d[14] = d[15] = 0;

    /* Write them as many times necessary */
    n >>= 5;

    while(n--) {
        __asm__("pref @%0" : : "r"(d));
        d += 8;
    }

    /* Wait for both store queues to complete */
    d = (unsigned int *)MEM_AREA_SQ_BASE;
    d[0] = d[8] = 0;
}

/* copies n bytes from src to dest, dest must be 32-byte aligned */
void * sq_cpy(void *dest, const void *src, int n) {
    unsigned int *d = (unsigned int *)(void *)
                      (MEM_AREA_SQ_BASE | (((unsigned long)dest) & 0x03ffffe0));
    const unsigned int *s = src;

    /* Set store queue memory area as desired */
    QACR0 = ((((unsigned int)dest) >> 26) << 2) & 0x1c;
    QACR1 = ((((unsigned int)dest) >> 26) << 2) & 0x1c;

    /* fill/write queues as many times necessary */
    n >>= 5;

    while(n--) {
        __asm__("pref @%0" : : "r"(s + 8));  /* prefetch 32 bytes for next loop */
        d[0] = *(s++);
        d[1] = *(s++);
        d[2] = *(s++);
        d[3] = *(s++);
        d[4] = *(s++);
        d[5] = *(s++);
        d[6] = *(s++);
        d[7] = *(s++);
        __asm__("pref @%0" : : "r"(d));
        d += 8;
    }

    /* Wait for both store queues to complete */
    d = (unsigned int *)MEM_AREA_SQ_BASE;
    d[0] = d[8] = 0;

    return dest;
}

/* fills n bytes at s with byte c, s must be 32-byte aligned */
void * sq_set(void *s, uint32 c, int n) {
    unsigned int *d = (unsigned int *)(void *)
                      (MEM_AREA_SQ_BASE | (((unsigned long)s) & 0x03ffffe0));

    /* Set store queue memory area as desired */
    QACR0 = ((((unsigned int)s) >> 26) << 2) & 0x1c;
    QACR1 = ((((unsigned int)s) >> 26) << 2) & 0x1c;

    /* duplicate low 8-bits of c into high 24-bits */
    c = c & 0xff;
    c = (c << 24) | (c << 16) | (c << 8) | c;

    /* Fill both store queues with c */
    d[0] = d[1] = d[2] = d[3] = d[4] = d[5] = d[6] = d[7] =
                                           d[8] = d[9] = d[10] = d[11] = d[12] = d[13] = d[14] = d[15] = c;

    /* Write them as many times necessary */
    n >>= 5;

    while(n--) {
        __asm__("pref @%0" : : "r"(d));
        d += 8;
    }

    /* Wait for both store queues to complete */
    d = (unsigned int *)MEM_AREA_SQ_BASE;
    d[0] = d[8] = 0;

    return s;
}

/* fills n bytes at s with short c, s must be 32-byte aligned */
void * sq_set16(void *s, uint32 c, int n) {
    unsigned int *d = (unsigned int *)(void *)
                      (MEM_AREA_SQ_BASE | (((unsigned long)s) & 0x03ffffe0));

    /* Set store queue memory area as desired */
    QACR0 = ((((unsigned int)s) >> 26) << 2) & 0x1c;
    QACR1 = ((((unsigned int)s) >> 26) << 2) & 0x1c;

    /* duplicate low 16-bits of c into high 16-bits */
    c = c & 0xffff;
    c = (c << 16) | c;

    /* Fill both store queues with c */
    d[0] = d[1] = d[2] = d[3] = d[4] = d[5] = d[6] = d[7] =
                                           d[8] = d[9] = d[10] = d[11] = d[12] = d[13] = d[14] = d[15] = c;

    /* Write them as many times necessary */
    n >>= 5;

    while(n--) {
        __asm__("pref @%0" : : "r"(d));
        d += 8;
    }

    /* Wait for both store queues to complete */
    d = (unsigned int *)MEM_AREA_SQ_BASE;
    d[0] = d[8] = 0;

    return s;
}

/* fills n bytes at s with int c, s must be 32-byte aligned */
void * sq_set32(void *s, uint32 c, int n) {
    unsigned int *d = (unsigned int *)(void *)
                      (MEM_AREA_SQ_BASE | (((unsigned long)s) & 0x03ffffe0));

    /* Set store queue memory area as desired */
    QACR0 = ((((unsigned int)s) >> 26) << 2) & 0x1c;
    QACR1 = ((((unsigned int)s) >> 26) << 2) & 0x1c;

    /* Fill both store queues with c */
    d[0] = d[1] = d[2] = d[3] = d[4] = d[5] = d[6] = d[7] =
                                           d[8] = d[9] = d[10] = d[11] = d[12] = d[13] = d[14] = d[15] = c;

    /* Write them as many times necessary */
    n >>= 5;

    while(n--) {
        __asm__("pref @%0" : : "r"(d));
        d += 8;
    }

    /* Wait for both store queues to complete */
    d = (unsigned int *)MEM_AREA_SQ_BASE;
    d[0] = d[8] = 0;

    return s;
}
