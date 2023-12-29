/* KallistiOS ##version##

   spu.c
   Copyright (C) 2000, 2001 Megan Potter
   Copyright (C) 2023 Ruslan Rostovtsev
 */

#include <dc/spu.h>
#include <dc/g2bus.h>
#include <dc/sq.h>
#include <arch/timer.h>

/*

This module handles the sound processor unit (SPU) of the Dreamcast system.
The processor is a Yamaha AICA, which is powered by an ARM7 RISC core.
To operate the CPU, you simply put it into reset, load a program and
potentially some data into the sound ram, and then let it out of reset. The
ARM will then start executing your code.

In the interests of simplifying the programmer's task, KallistiOS has
made available several default sound programs. One of them is designed to
play MIDI-style tracker data (converted S3M/XM/MOD/MIDI/etc) and the other
is designed to play buffered sound data. Each of these has an associated
API that can be used from the SH-4. Note that the act of referencing
either in your program statically causes them to be linked into the
kernel; so don't use them if you don't need to =).

*/

/* Some convenience macros */
#define SNDREGADDR(x) (0xa0700000 + (x))
#define CHNREGADDR(chn, x) SNDREGADDR(0x80*(chn) + (x))

/* memcpy and memset designed for sound RAM; for addresses, don't
   bother to include the 0xa0800000 offset that is implied. 'length'
   must be a multiple of 4, but if it is not it will be rounded up. */
void spu_memload(uintptr_t dst, void *src_void, size_t length) {
    uint8_t *src = (uint8_t *)src_void;

    /* Make sure it's an even number of 32-bit words and convert the
       count to a 32-bit word count */
    length = (length + 3) >> 2;

    /* Add in the SPU RAM base */
    dst += SPU_RAM_UNCACHED_BASE;

    while(length > 8) {
        g2_fifo_wait();
        g2_write_block_32((uint32_t *)src, dst, 8);

        src += 8 * 4;
        dst += 8 * 4;
        length -= 8;
    }

    if(length > 0) {
        g2_fifo_wait();
        g2_write_block_32((uint32_t *)src, dst, length);
    }
}

void spu_memload_sq(uintptr_t dst, void *src_void, size_t length) {
    uint8_t *src = (uint8_t *)src_void;
    int aligned_len;
    g2_ctx_t ctx;

    /* Round up to the nearest multiple of 4 */
    if(length & 3) {
        length = (length + 4) & ~3;
    }

    /* Using SQs for all that is divisible by 32 */
    aligned_len = length & ~31;
    length &= 31;

    /* Add in the SPU RAM base (cached area) */
    dst += SPU_RAM_BASE;

    /* Make sure the FIFOs are empty */
    ctx = g2_lock();

    sq_cpy((void *)dst, src, aligned_len);

    g2_unlock(ctx);

    if(length > 0) {
        /* Make sure the destination is in a non-cached area */
        dst |= MEM_AREA_P2_BASE;
        dst += aligned_len;
        src += aligned_len;
        g2_fifo_wait();
        g2_write_block_32((uint32_t *)src, dst, length >> 2);
    }
}

void spu_memread(void *dst_void, uintptr_t src, size_t length) {
    uint8_t *dst = (uint8_t *)dst_void;

    /* Make sure it's an even number of 32-bit words and convert the
       count to a 32-bit word count */
    length = (length + 3) >> 2;

    /* Add in the SPU RAM base */
    src += SPU_RAM_UNCACHED_BASE;

    while(length > 8) {
        g2_fifo_wait();
        g2_read_block_32((uint32_t *)dst, src, 8);

        src += 8 * 4;
        dst += 8 * 4;
        length -= 8;
    }

    if(length > 0) {
        g2_fifo_wait();
        g2_read_block_32((uint32_t *)dst, src, length);
    }
}

void spu_memset(uintptr_t dst, uint32_t what, size_t length) {
    uint32_t  blank[8];
    int i;

    /* Make sure it's an even number of 32-bit words and convert the
       count to a 32-bit word count */
    length = (length + 3) >> 2;

    /* Initialize the array */
    for(i = 0; i < 8; i++)
        blank[i] = what;

    /* Add in the SPU RAM base */
    dst += SPU_RAM_UNCACHED_BASE;

    while(length > 8) {
        g2_fifo_wait();
        g2_write_block_32(blank, dst, 8);

        dst += 8 * 4;
        length -= 8;
    }

    if(length > 0) {
        g2_fifo_wait();
        g2_write_block_32(blank, dst, length);
    }
}

/* Reset the AICA channel registers */
void spu_reset_chans(void) {
    int i;
    g2_fifo_wait();
    g2_write_32(SNDREGADDR(0x2800), 0);

    for(i = 0; i < 64; i++) {
        if(!(i % 4)) g2_fifo_wait();

        g2_write_32(CHNREGADDR(i, 0), 0x8000);
        g2_write_32(CHNREGADDR(i, 20), 0x1f);
    }

    g2_fifo_wait();
    g2_write_32(SNDREGADDR(0x2800), 0x000f);
}

/* Enable/disable the SPU; note that disable implies reset of the
   ARM CPU core. */
void spu_enable(void) {
    /* Reset all the channels */
    spu_reset_chans();

    /* Start the ARM processor */
    g2_write_32(SNDREGADDR(0x2c00), g2_read_32(SNDREGADDR(0x2c00)) & ~1);
}

void spu_disable(void) {
    /* Stop the ARM processor */
    g2_write_32(SNDREGADDR(0x2c00), g2_read_32(SNDREGADDR(0x2c00)) | 1);

    /* Make sure we didn't leave any notes running */
    spu_reset_chans();
}

/* Set CDDA volume: values are 0-15 */
void spu_cdda_volume(int left_volume, int right_volume) {
    if(left_volume > 15)
        left_volume = 15;

    if(right_volume > 15)
        right_volume = 15;

    g2_fifo_wait();
    g2_write_32(SNDREGADDR(0x2040),
                (g2_read_32(SNDREGADDR(0x2040)) & ~0xff00) | (left_volume << 8));
    g2_write_32(SNDREGADDR(0x2044),
                (g2_read_32(SNDREGADDR(0x2044)) & ~0xff00) | (right_volume << 8));
}

void spu_cdda_pan(int left_pan, int right_pan) {
    if(left_pan < 16)
        left_pan = ~(left_pan - 16);

    left_pan &= 0x1f;

    if(right_pan < 16)
        right_pan = ~(right_pan - 16);

    right_pan &= 0x1f;

    g2_fifo_wait();
    g2_write_32(SNDREGADDR(0x2040),
                (g2_read_32(SNDREGADDR(0x2040)) & ~0xff) | (left_pan << 0));
    g2_write_32(SNDREGADDR(0x2044),
                (g2_read_32(SNDREGADDR(0x2044)) & ~0xff) | (right_pan << 0));
}

/* Initialize CDDA stuff */
static void spu_cdda_init(void) {
    spu_cdda_volume(15, 15);
    spu_cdda_pan(0, 31);
}

/* Set master volume (0..15) and mono/stereo settings */
void spu_master_mixer(int volume, int stereo) {
    g2_fifo_wait();
    g2_write_32(SNDREGADDR(0x2800), volume | (stereo ? 0 : 0x8000));
}

/* Initialize the SPU; by default it will be left in a state of
   reset until you upload a program. */
int spu_init(void) {
    /* Stop the ARM */
    spu_disable();

    /* Clear out sound RAM */
    spu_memset(0, 0, 0x200000);

    /* Load a default "program" into the SPU that just executes
       an infinite loop, so that CD audio works. */
    g2_fifo_wait();
    g2_write_32(SPU_RAM_UNCACHED_BASE, 0xeafffff8);

    /* Start the SPU again */
    spu_enable();

    /* Wait a few clocks */
    timer_spin_sleep(10);

    /* Initialize CDDA channels */
    spu_cdda_init();

    return 0;
}

/* Shutdown SPU */
int spu_shutdown(void) {
    spu_disable();
    spu_memset(0, 0, 0x200000);
    return 0;
}

int spu_dma_transfer(void *from, uintptr_t dest, size_t length, int block,
                     g2_dma_callback_t callback, void *cbdata) {
    /* Adjust destination to SPU RAM */
    dest += SPU_RAM_BASE;

    return g2_dma_transfer(from, (void *) dest, length, block, callback, cbdata, 0,
                           0, G2_DMA_CHAN_SPU, 0);
}
