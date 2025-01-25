/* KallistiOS ##version##

   snd_iface.c
   Copyright (C) 2000-2002 Megan Potter
   Copyright (C) 2024 Ruslan Rostovtsev

   SH-4 support routines for accessing the AICA via the standard KOS driver
*/

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include <kos/mutex.h>
#include <arch/timer.h>
#include <dc/g2bus.h>
#include <dc/spu.h>
#include <dc/sound/sound.h>

#include "arm/aica_cmd_iface.h"

/* Are we initted? */
static int initted = 0;

/* This will come from a separately linked object file */
extern uintptr_t snd_stream_drv;
extern uintptr_t snd_stream_drv_end;

/* The queue processing mutex for snd_sh4_to_aica_start and snd_sh4_to_aica_stop.
   There are some cases like stereo stream control + stereo sfx control
   at the same time in separate threads. */
static mutex_t queue_proc_mutex = MUTEX_INITIALIZER;

/* Initialize driver; note that this replaces the AICA program so that
   if you had anything else going on, it's gone now! */
int snd_init(void) {
    size_t amt;

    /* Finish loading the stream driver */
    if(!initted) {
        spu_disable();
        spu_memset_sq(0, 0, AICA_RAM_START);
        amt = snd_stream_drv_end - snd_stream_drv;

        if(amt % 4)
            amt = (amt + 4) & ~3;

        dbglog(DBG_DEBUG, "snd_init(): loading %u bytes into SPU RAM\n", amt);
        spu_memload_sq(0, (void* )snd_stream_drv, amt);

        /* Enable the AICA and give it a few ms to start up */
        spu_enable();
        timer_spin_sleep(10);

        /* Initialize the RAM allocator */
        snd_mem_init(AICA_RAM_START);
    }

    initted = 1;

    return 0;
}

/* Shut everything down and free mem */
void snd_shutdown(void) {
    if(initted) {
        spu_disable();
        snd_mem_shutdown();
        initted = 0;
    }
}

/* Submit a request to the SH4->AICA queue; size is in uint32's */
int snd_sh4_to_aica(void *packet, uint32_t size) {
    uint32_t qa, bot, start, top, *pkt32, cnt;
    g2_ctx_t ctx;
    assert_msg(size < AICA_CMD_MAX_SIZE, "SH4->AICA packets may not be >256 uint32's long");

    ctx = g2_lock();

    /* Set these up for reference */
    qa = SPU_RAM_UNCACHED_BASE + AICA_MEM_CMD_QUEUE;
    assert_msg(g2_read_32_raw(qa + offsetof(aica_queue_t, valid)), "Queue is not yet valid");

    bot = SPU_RAM_UNCACHED_BASE + g2_read_32_raw(qa + offsetof(aica_queue_t, data));
    top = bot + g2_read_32_raw(qa + offsetof(aica_queue_t, size));
    start = bot + g2_read_32_raw(qa + offsetof(aica_queue_t, head));
    pkt32 = (uint32_t *)packet;
    cnt = 0;

    while(size-- > 0) {
        /* Fifo wait if necessary */
        if((cnt++ & 7) == 0)
            g2_fifo_wait();

        /* Write the next dword */
        g2_write_32_raw(start, *pkt32++);

        /* Move our counters */
        start += 4;

        if(start >= top)
            start = bot;
    }

    /* Finally, write a new head value to signify that we've added
       a packet for it to process */
    if((cnt & 7) == 0)
        g2_fifo_wait();

    g2_write_32_raw(qa + offsetof(aica_queue_t, head), start - bot);
    g2_unlock(ctx);

    /* We could wait until head == tail here for processing, but there's
       not really much point; it'll just slow things down. */
    return 0;
}

/* Start processing requests in the queue */
void snd_sh4_to_aica_start(void) {
    g2_write_32(SPU_RAM_UNCACHED_BASE + AICA_MEM_CMD_QUEUE + offsetof(aica_queue_t, process_ok), 1);
    mutex_unlock(&queue_proc_mutex);
}

/* Stop processing requests in the queue */
void snd_sh4_to_aica_stop(void) {
    mutex_lock(&queue_proc_mutex);
    g2_write_32(SPU_RAM_UNCACHED_BASE + AICA_MEM_CMD_QUEUE + offsetof(aica_queue_t, process_ok), 0);
}

/* Transfer one packet of data from the AICA->SH4 queue. Expects to
   find AICA_CMD_MAX_SIZE dwords of space available. Returns -1
   if failure, 0 for no packets available, 1 otherwise. Failure
   might mean a permanent failure since the queue is probably out of sync. */
int snd_aica_to_sh4(void *packetout) {
    uint32  bot, start, stop, top, size, cnt, *pkt32;
    g2_ctx_t ctx = g2_lock();

    /* Set these up for reference */
    bot = SPU_RAM_UNCACHED_BASE + AICA_MEM_RESP_QUEUE;
    assert_msg(g2_read_32_raw(bot + offsetof(aica_queue_t, valid)), "Queue is not yet valid");

    top = SPU_RAM_UNCACHED_BASE + AICA_MEM_RESP_QUEUE + g2_read_32_raw(bot + offsetof(aica_queue_t, size));
    start = SPU_RAM_UNCACHED_BASE + AICA_MEM_RESP_QUEUE + g2_read_32_raw(bot + offsetof(aica_queue_t, tail));
    stop = SPU_RAM_UNCACHED_BASE + AICA_MEM_RESP_QUEUE + g2_read_32_raw(bot + offsetof(aica_queue_t, head));
    cnt = 0;
    pkt32 = (uint32_t *)packetout;

    /* Is there anything? */
    if(start == stop) {
        g2_unlock(ctx);
        return 0;
    }

    /* Check for packet size overflow */
    size = g2_read_32_raw(start + offsetof(aica_cmd_t, size));

    if(size >= AICA_CMD_MAX_SIZE) {
        g2_unlock(ctx);
        dbglog(DBG_ERROR, "snd_aica_to_sh4(): packet larger than %d dwords\n", AICA_CMD_MAX_SIZE);
        return -1;
    }

    /* Find stop point for this packet */
    stop = start + size * 4;

    if(stop > top)
        stop -= top - (SPU_RAM_UNCACHED_BASE + AICA_MEM_RESP_QUEUE);

    while(start != stop) {
        /* Fifo wait if necessary */
        if((cnt++ & 7) == 0)
            g2_fifo_wait();

        /* Read the next dword */
        *pkt32++ = g2_read_32_raw(start);

        /* Move our counters */
        start += 4;

        if(start >= top)
            start = bot;
    }

    /* Finally, write a new tail value to signify that we've removed a packet */
    if((cnt & 7) == 0)
        g2_fifo_wait();

    g2_write_32_raw(bot + offsetof(aica_queue_t, tail), start - (SPU_RAM_UNCACHED_BASE + AICA_MEM_RESP_QUEUE));
    g2_unlock(ctx);

    return 1;
}

/* Poll for responses from the AICA. We assume here that we're not
   running in an interrupt handler (thread perhaps, of whoever
   is using us). */
void snd_poll_resp(void) {
    int rv;
    uint32_t pkt[AICA_CMD_MAX_SIZE];
    aica_cmd_t *pktcmd;

    pktcmd = (aica_cmd_t *)pkt;

    while((rv = snd_aica_to_sh4(pkt)) > 0) {
        dbglog(DBG_DEBUG, "snd_poll_resp(): Received packet id %08lx, ts %08lx from AICA\n",
               pktcmd->cmd, pktcmd->timestamp);
    }

    if(rv < 0)
        dbglog(DBG_ERROR, "snd_poll_resp(): snd_aica_to_sh4 failed, giving up\n");
}

uint16_t snd_get_pos(unsigned int ch) {
    return g2_read_32(SPU_RAM_UNCACHED_BASE + AICA_CHANNEL(ch) + offsetof(aica_channel_t, pos)) & 0xffff;
}

bool snd_is_playing(unsigned int ch) {
    return g2_read_32(MEM_AREA_P2_BASE + 0x00700000 + 0x80 * ch) & AICA_CHANNEL_KEYONB;
}
