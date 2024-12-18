/* KallistiOS ##version##

   maple_queue.c
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2015 Lawrence Sebald
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <dc/maple.h>
#include <arch/irq.h>
#include <arch/memory.h>

/* Send all queued frames */
void maple_queue_flush(void) {
    int     cnt, amt;
    uint32      *out, *last;
    maple_frame_t   *i;

    cnt = amt = 0;
    out = (uint32 *)maple_state.dma_buffer;
    last = NULL;

    /* Make sure we end up with space for the gun enable command... */
    if(maple_state.gun_port > -1)
        amt = 12;

    /* Go through and process each frame... */
    TAILQ_FOREACH(i, &maple_state.frame_queue, frameq) {
        /* Are we running out of space? */
        if((i->length * 4 + amt) > MAPLE_DMA_SIZE)
            break;

        /* Is this frame stale? */
        if(i->state != MAPLE_FRAME_UNSENT)
            continue;

        i->state = MAPLE_FRAME_SENT;

        /* Save the last descriptor head for the "last" flag */
        last = out;

        /* First word: message length and destination port */
        *out++ = i->length | (i->dst_port << 16);

        /* Second word: receive buffer physical address */
        *out++ = ((uint32)i->recv_buf) & MEM_AREA_CACHE_MASK;

        /* Third word: command, addressing, packet length */
        *out++ = (i->cmd & 0xff) | (maple_addr(i->dst_port, i->dst_unit) << 8)
                 | ((i->dst_port << 6) << 16)
                 | ((i->length & 0xff) << 24);

        /* Finally, parameter words, if any */
        if(i->length > 0) {
            assert(i->send_buf != NULL);
            memcpy(out, i->send_buf, i->length * 4);
            out += i->length;
        }

        cnt++;
        amt += i->length * 4;
    }

    /* Are we entering gun mode this frame? */
    if(maple_state.gun_port > -1) {
        last = out;
        *out++ = 0x200 | (maple_state.gun_port << 16);
        *out++ = 0;
        *out++ = 0;
        cnt++;
    }

    /* Did we actually do anything...? */
    if(cnt > 0) {
        /* Tack on the "last" bit to the last one */
        assert(last != NULL);
        *last |= 0x80000000;

        /* Start a DMA transfer */
        maple_dma_addr(maple_state.dma_buffer);
        maple_dma_start();
        maple_state.dma_in_progress = 1;
    }
}

/* Submit a frame for queueing; see header for notes */
int maple_queue_frame(maple_frame_t *frame) {
    uint32 save = 0;

    /* Don't add it twice */
    if(frame->queued)
        return -1;

    /* If we're not running inside an interrupt, then disable interrupts
       so the list won't change underneath us */
    if(!irq_inside_int())
        save = irq_disable();

    /* Assign it a device, if applicable */
    frame->dev = maple_enum_dev(frame->dst_port, frame->dst_unit);

    /* Put it on the queue */
    TAILQ_INSERT_TAIL(&maple_state.frame_queue, frame, frameq);
    frame->queued = 1;

    /* Restore interrupts */
    if(!irq_inside_int())
        irq_restore(save);

    return 0;
}

/* Remove a used frame from the queue */
int maple_queue_remove(maple_frame_t *frame) {
    uint32 save = 0;

    /* Don't remove twice */
    if(!frame->queued)
        return -1;

    /* If we're not running inside an interrupt, then disable interrupts
       so the list won't change underneath us */
    if(!irq_inside_int())
        save = irq_disable();

    /* Remove it from the queue */
    TAILQ_REMOVE(&maple_state.frame_queue, frame, frameq);
    frame->queued = 0;

    /* Restore interrupts */
    if(!irq_inside_int())
        irq_restore(save);

    return 0;
}

/* Initialize a new frame to prepare it to be placed on the queue; call
   this _before_ you fill it in */
/* Note on buffer alignments:
   As before, with the old maple system, if I 32-byte align everything
   then some memory seems to get overwritten before/after the buffer. In
   the old system I put it inside a big chunk of memory so it couldn't do
   that, and that seems to be the only working fix here too. *shrug* */
void maple_frame_init(maple_frame_t *frame) {
    uint32 buf_ptr;

    assert(frame->state == MAPLE_FRAME_UNSENT);
    assert(!frame->queued);

    /* Setup the buffer pointer (64-byte align and force -> P2) */
    buf_ptr = (uint32)frame->recv_buf_arr;

    if(buf_ptr & 0x1f)
        buf_ptr = (buf_ptr & ~0x1f) + 0x20;

#if MAPLE_DMA_DEBUG
    buf_ptr += 512;
#endif
    buf_ptr = (buf_ptr & MEM_AREA_CACHE_MASK) | MEM_AREA_P2_BASE;
    frame->recv_buf = (uint8*)buf_ptr;

    /* Clear out the receive buffer */
#if MAPLE_DMA_DEBUG
    maple_sentinel_setup(frame->recv_buf - 512, 1024 + 1024);
#else
    memset(frame->recv_buf, 0, 1024);
#endif

    /* Initialize other state stuff */
    frame->cmd = -1;
    frame->dst_port = frame->dst_unit = 0;
    frame->length = 0;
    frame->queued = 0;
    frame->dev = NULL;
    frame->send_buf = NULL;
    frame->callback = NULL;
}

/* Lock a frame so that someone else can't use it in the mean time; if the
   frame is already locked, an error will be returned. */
int maple_frame_lock(maple_frame_t *frame) {
    uint32  save = 0;
    int rv;

    if(!irq_inside_int())
        save = irq_disable();

    if(frame->queued || frame->state != MAPLE_FRAME_VACANT)
        rv = -1;
    else {
        frame->state = MAPLE_FRAME_UNSENT;
        rv = 0;
    }

    if(!irq_inside_int())
        irq_restore(save);

    return rv;
}

/* Unlock a frame */
void maple_frame_unlock(maple_frame_t *frame) {
    assert(frame->state == MAPLE_FRAME_RESPONDED);
    frame->state = MAPLE_FRAME_VACANT;
}
