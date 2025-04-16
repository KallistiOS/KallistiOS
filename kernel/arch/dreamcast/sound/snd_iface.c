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
#include <dc/aram.h>
#include <dc/g2bus.h>
#include <dc/spu.h>
#include <dc/sound/sound.h>

#include "arm/aica_cmd_iface.h"

/* Channels status register bits */
#define AICA_CHANNEL_KEYONEX   0x8000
#define AICA_CHANNEL_KEYONB    0x4000

struct aica_header aica_header = {
    .cmd_queue = (struct aica_queue *)AICA_MEM_CMD_QUEUE,
    .resp_queue = (struct aica_queue *)AICA_MEM_RESP_QUEUE,
    .channels = (struct aica_channel *)AICA_MEM_CHANNELS,
    .buffer = (void *)AICA_RAM_START,
    .buffer_size = AICA_RAM_END - AICA_RAM_START,
};

/* Are we initted? */
static int initted = 0;

/* This will come from a separately linked object file */
extern uint8_t snd_stream_drv[];
extern uint8_t snd_stream_drv_end[];

/* The queue processing mutex for snd_sh4_to_aica_start and snd_sh4_to_aica_stop.
   There are some cases like stereo stream control + stereo sfx control
   at the same time in separate threads. */
static mutex_t queue_proc_mutex = MUTEX_INITIALIZER;

/* Initialize driver; note that this replaces the AICA program so that
   if you had anything else going on, it's gone now! */
int snd_init(void) {
    int amt;

    /* Finish loading the stream driver */
    if(!initted) {
        spu_disable();
        spu_memset_sq(0, 0, AICA_RAM_START);
        amt = snd_stream_drv_end - snd_stream_drv;

        if(amt % 4)
            amt = (amt + 4) & ~3;

        dbglog(DBG_DEBUG, "snd_init(): loading %d bytes into SPU RAM\n", amt);
        spu_memload_sq(0, snd_stream_drv, amt);

        /* Enable the AICA and give it a few ms to start up */
        spu_enable();
        timer_spin_sleep(10);

        /* Initialize the RAM allocator */
        snd_mem_init((uint32_t)aica_header.buffer);
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
    aram_addr_t bot, start, top;
    aica_queue_t cmd_queue;

    assert_msg(size < AICA_CMD_MAX_SIZE, "SH4->AICA packets may not be >256 uint32's long");

    /* Read the command queue's structure */
    aram_read(&cmd_queue, (aram_addr_t)aica_header.cmd_queue, sizeof(cmd_queue));

    assert_msg(cmd_queue.valid, "Queue is not yet valid");

    bot = (aram_addr_t)cmd_queue.data;
    top = bot + cmd_queue.size;
    start = bot + cmd_queue.head;

    /* From now on count the size as bytes */
    size *= 4;

    if (size > top - start) {
        aram_write(start, packet, top - start);

        size -= top - start;
        packet += top - start;
        start = bot;
    }

    aram_write(start, packet, size);
    start += size;

    /* Finally, write a new head value to signify that we've added
       a packet for it to process */
    aram_write_32((aram_addr_t)aica_header.cmd_queue + offsetof(aica_queue_t, head),
                  start - bot);

    /* We could wait until head == tail here for processing, but there's
       not really much point; it'll just slow things down. */
    return 0;
}

/* Start processing requests in the queue */
void snd_sh4_to_aica_start(void) {
    aram_write_32((aram_addr_t)aica_header.cmd_queue + offsetof(aica_queue_t, process_ok), 1);
    mutex_unlock(&queue_proc_mutex);
}

/* Stop processing requests in the queue */
void snd_sh4_to_aica_stop(void) {
    mutex_lock(&queue_proc_mutex);
    aram_write_32((aram_addr_t)aica_header.cmd_queue + offsetof(aica_queue_t, process_ok), 0);
}

/* Transfer one packet of data from the AICA->SH4 queue. Expects to
   find AICA_CMD_MAX_SIZE dwords of space available. Returns -1
   if failure, 0 for no packets available, 1 otherwise. Failure
   might mean a permanent failure since the queue is probably out of sync. */
int snd_aica_to_sh4(void *packetout) {
    aram_addr_t bot, top, start, stop;
    aica_queue_t resp_queue;
    uint32_t size;

    /* Read the response queue's structure */
    aram_read(&resp_queue, (aram_addr_t)aica_header.resp_queue, sizeof(resp_queue));

    assert_msg(resp_queue.valid, "Queue is not yet valid");

    bot = (aram_addr_t)resp_queue.data;
    top = bot + resp_queue.size;
    start = bot + resp_queue.tail;
    stop = bot + resp_queue.head;

    /* Is there anything? */
    if(start == stop)
        return 0;

    /* Check for packet size overflow */
    size = aram_read_32(start + offsetof(aica_cmd_t, size));

    if(size >= AICA_CMD_MAX_SIZE) {
        dbglog(DBG_ERROR, "snd_aica_to_sh4(): packet larger than %d dwords\n", AICA_CMD_MAX_SIZE);
        return -1;
    }

    /* Count in bytes now */
    size *= 4;

    /* Find stop point for this packet */
    stop = start + size;

    if(stop > top) {
        aram_read(packetout, start, top - start);

        size -= top - start;
        packetout += top - start;
        start = bot;
    }

    aram_read(packetout, start, size);
    start += size;

    /* Finally, write a new tail value to signify that we've removed a packet */
    aram_write_32((aram_addr_t)aica_header.resp_queue + offsetof(aica_queue_t, tail),
                  start - bot);

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
    return aram_read_32((aram_addr_t)aica_header.channels +
                        ch * sizeof(*aica_header.channels) +
                        offsetof(aica_channel_t, pos));
}

bool snd_is_playing(unsigned int ch) {
    return g2_read_32(MEM_AREA_P2_BASE + 0x00700000 + 0x80 * ch) & AICA_CHANNEL_KEYONB;
}
