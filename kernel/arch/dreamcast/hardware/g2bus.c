/* KallistiOS ##version##

   g2bus.c
   (c)2000-2002 Megan Potter
*/

/*

  This module handles low-level access to the DC's "G2" bus, which handles
  communication with the SPU (AICA) and the expansion port. One must be
  very careful with this bus, as it requires 32-bit access for most
  things, FIFO checking for PIO access, suspended DMA for PIO access,
  etc, etc... very picky =)

  Thanks to Marcus Comstedt and Marcus Brown for the info about when
  to lock/suspend DMA/etc.

 */

#include <string.h>
#include <stdio.h>
#include <dc/g2bus.h>
#include <arch/spinlock.h>

/* Always use these functions to access G2 bus memory (includes the SPU
   and the expansion port, e.g., BBA) */

/* Read one byte from G2 */
uint8_t g2_read_8(uintptr_t address) {
    g2_ctx_t ctx;
    uint8 out;

    ctx = g2_lock();
    out = *((vuint8*)address);
    g2_unlock(ctx);

    return out;
}

/* Write one byte to G2 */
void g2_write_8(uintptr_t address, uint8_t value) {
    g2_ctx_t ctx;

    ctx = g2_lock();
    *((vuint8*)address) = value;
    g2_unlock(ctx);
}

/* Read one word from G2 */
uint16_t g2_read_16(uintptr_t address) {
    g2_ctx_t ctx;
    uint16 out;

    ctx = g2_lock();
    out = *((vuint16*)address);
    g2_unlock(ctx);

    return out;
}

/* Write one word to G2 */
void g2_write_16(uintptr_t address, uint16_t value) {
    g2_ctx_t ctx;

    ctx = g2_lock();
    *((vuint16*)address) = value;
    g2_unlock(ctx);
}

/* Read one dword from G2 */
uint32_t g2_read_32(uintptr_t address) {
    g2_ctx_t ctx;
    uint32 out;

    ctx = g2_lock();
    out = *((vuint32*)address);
    g2_unlock(ctx);

    return out;
}

/* Write one dword to G2 */
void g2_write_32(uintptr_t address, uint32_t value) {
    g2_ctx_t ctx;

    ctx = g2_lock();
    *((vuint32*)address) = value;
    g2_unlock(ctx);
}

/* Read a block of 8-bit values from G2 */
void g2_read_block_8(uint8_t * output, uintptr_t address, size_t amt) {
    const vuint8 * input = (const vuint8 *)address;
    g2_ctx_t ctx;

    ctx = g2_lock();

    while(amt--) {
        *output++ = *input++;
    }

    g2_unlock(ctx);
}

/* Write a block 8-bit values to G2 */
void g2_write_block_8(const uint8 * input, uintptr_t address, size_t amt) {
    vuint8 * output = (vuint8 *)address;
    g2_ctx_t ctx;

    ctx = g2_lock();

    while(amt--) {
        *output++ = *input++;
    }

    g2_unlock(ctx);
}

/* Read a block of 16-bit values from G2 */
void g2_read_block_16(uint16_t * output, uintptr_t address, size_t amt) {
    const vuint16 * input = (const vuint16 *)address;
    g2_ctx_t ctx;

    ctx = g2_lock();

    while(amt--) {
        *output++ = *input++;
    }

    g2_unlock(ctx);
}

/* Write a block of 16-bit values to G2 */
void g2_write_block_16(const uint16_t * input, uintptr_t address, size_t amt) {
    vuint16 * output = (vuint16 *)address;
    g2_ctx_t ctx;

    ctx = g2_lock();

    while(amt--) {
        *output++ = *input++;
    }

    g2_unlock(ctx);
}

/* Read a block of 32-bit values from G2 */
void g2_read_block_32(uint32_t * output, uintptr_t address, size_t amt) {
    const vuint32 * input = (const vuint32 *)address;
    g2_ctx_t ctx;

    ctx = g2_lock();

    while(amt--) {
        *output++ = *input++;
    }

    g2_unlock(ctx);
}

/* Write a block of 32-bit values to G2 */
void g2_write_block_32(const uint32_t * input, uintptr_t address, size_t amt) {
    vuint32 * output = (vuint32 *)address;
    g2_ctx_t ctx;

    ctx = g2_lock();

    while(amt--) {
        *output++ = *input++;
    }

    g2_unlock(ctx);
}

/* A memset-like function for G2 */
void g2_memset_8(uintptr_t address, uint8_t c, size_t amt) {
    vuint8 * output = (vuint8 *)address;
    g2_ctx_t ctx;

    ctx = g2_lock();

    while(amt--) {
        *output++ = c;
    }

    g2_unlock(ctx);
}

/* When writing to the SPU RAM, this is required at least every 8 32-bit
   writes that you execute */
void g2_fifo_wait(void) {
    while(FIFO_STATUS & (FIFO_AICA | FIFO_G2));
}
