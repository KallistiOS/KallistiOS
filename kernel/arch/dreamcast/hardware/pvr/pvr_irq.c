/* KallistiOS ##version##

   pvr_irq.c
   Copyright (C)2002,2004 Megan Potter

 */

#include <assert.h>
#include <dc/pvr.h>
#include <dc/asic.h>
#include "pvr_internal.h"

#include <kos/dbglog.h>
#include <kos/genwait.h>
#include <kos/regfield.h>

#include <stdio.h>

/*
   PVR interrupt handler; the way things are setup, we're gonna get
   one of these for each full vertical refresh and at the completion
   of TA data acceptance. The timing here is pretty critical. We need
   to flip pages during a vertical blank, and then signal to the program
   that it's ok to start playing with TA registers again, or we waste
   rendering time.
*/

// Find the next list to DMA out. If we have none left to do, then free
// the DMA channel. Otherwise, start the DMA and chain back to us upon
// completion.
static void dma_next_list(void *thread) {
    volatile pvr_dma_buffers_t * b;
    unsigned int i;

    // Get the buffers the lists are sent from.
    b = pvr_state.dma_buffers + pvr_state.dma_target;

    if(pvr_state.lists_to_dma) {
        i = __builtin_ctz(pvr_state.lists_to_dma);

        // Mark this list as processed.
        pvr_state.lists_to_dma &= ~BIT(i);

        // Start the DMA transfer, chaining to ourselves.
        pvr_dma_load_ta(b->base[i], b->ptr[i], 0, dma_next_list, thread);
        return;
    }

    // Buffers are now empty again
    b->ready = 0;

    // If that was the last one, then free up the DMA channel.
    sem_signal((semaphore_t *)&pvr_state.dma_lock);
}

/* Send the given lists of RAM buffer target to the TA by DMA, in list
   order. Returns once the first DMA has started: the transfer ends in the
   background, and pvr_wait_dma() waits for it. A transfer started before
   (vertex or texture DMA) is waited for first. */
void pvr_start_dma(int target, uint32_t lists) {
    sem_wait((semaphore_t *)&pvr_state.dma_lock);

    pvr_state.dma_target = target;
    pvr_state.lists_to_dma = lists;

    // Begin DMAing the first list.
    dma_next_list(thd_get_current());
}

/* Wait until the DMA channel is free, as before the TA takes a list
   submitted directly after pvr_list_flush(). */
void pvr_wait_dma(void) {
    sem_wait((semaphore_t *)&pvr_state.dma_lock);
    sem_signal((semaphore_t *)&pvr_state.dma_lock);
}

static void pvr_render_lists(void) {
    if(pvr_state.ta_busy
       && !pvr_state.render_busy
       && (!pvr_state.render_completed || pvr_state.curr_to_texture)
       && pvr_state.lists_transferred == pvr_state.lists_enabled) {

        /* XXX Note:
           For some reason, the render must be started _before_ we sync
           to the new reg buffers. The only reasons I can think of for this
           are that there may be something in the reg sync that messes up
           the render in progress, or we are misusing some bits somewhere. */

        // Begin rendering from the dirty TA buffer into the clean
        // frame buffer.
        pvr_state.ta_target ^= pvr_state.vbuf_doublebuf;
        pvr_begin_queued_render();
        pvr_state.render_busy = 1;
        pvr_sync_stats(PVR_SYNC_RNDSTART);

        // Switch to the clean TA buffer.
        pvr_state.lists_transferred = 0;
        pvr_sync_reg_buffer();

        // The TA is no longer busy.
        pvr_state.ta_busy = 0;

        pvr_state.was_to_texture = pvr_state.curr_to_texture;

        // Signal the client code to continue onwards.
        genwait_wake_all((void *)&pvr_state.ta_busy);
        thd_schedule(true);
    }
}

void pvr_vblank_handler(uint32_t code, void *data) {
    (void)code;
    (void)data;

    pvr_sync_stats(PVR_SYNC_VBLANK);

    // If the render-done interrupt has fired then we are ready to flip to the
    // new frame buffer.
    if(pvr_state.render_completed) {
        // Handle PVR stats
        pvr_sync_stats(PVR_SYNC_PAGEFLIP);

        // Switch view address to the "good" buffer
        pvr_state.view_target ^= 1;

        pvr_sync_view();

        // Clear the render completed flag.
        pvr_state.render_completed = 0;
    }

    // We may have a pending render, that couldn't be done as the previous
    // render wasn't flipped yet; do it now.
    pvr_render_lists();
}

void pvr_int_handler(uint32_t code, void *data) {
    (void)data;

    // What kind of event did we get?
    switch(code) {
        case ASIC_EVT_PVR_OPAQUEDONE:
            pvr_state.lists_transferred |= BIT(PVR_LIST_OP_POLY);
            break;
        case ASIC_EVT_PVR_TRANSDONE:
            pvr_state.lists_transferred |= BIT(PVR_LIST_TR_POLY);
            break;
        case ASIC_EVT_PVR_OPAQUEMODDONE:
            pvr_state.lists_transferred |= BIT(PVR_LIST_OP_MOD);
            break;
        case ASIC_EVT_PVR_TRANSMODDONE:
            pvr_state.lists_transferred |= BIT(PVR_LIST_TR_MOD);
            break;
        case ASIC_EVT_PVR_PTDONE:
            pvr_state.lists_transferred |= BIT(PVR_LIST_PT_POLY);
            break;
        case ASIC_EVT_PVR_RENDERDONE_TSP:
            pvr_state.render_busy = 0;
            if(!pvr_state.was_to_texture)
                pvr_state.render_completed = 1;
            pvr_sync_stats(PVR_SYNC_RNDDONE);

            genwait_wake_all((void *)&pvr_state.render_busy);
            break;
    }

    /* Show register values on each interrupt */
    switch (code) {
        case ASIC_EVT_PVR_ISP_OUTOFMEM:
            dbglog(DBG_SOURCE(PVR_RENDER_DBG), "pvr_irq: ASIC_EVT_PVR_ISP_OUTOFMEM\n");
            break;

        case ASIC_EVT_PVR_STRIP_HALT:
            dbglog(DBG_SOURCE(PVR_RENDER_DBG), "pvr_irq: ASIC_EVT_PVR_STRIP_HALT\n");
            break;

        case ASIC_EVT_PVR_OPB_OUTOFMEM:
            dbglog(DBG_SOURCE(PVR_RENDER_DBG), "pvr_irq: ASIC_EVT_PVR_OPB_OUTOFMEM\n"
            "pvr_irq: PVR_TA_OPB_START: %08lx\n"
            "pvr_irq: PVR_TA_OPB_END: %08lx\n"
            "pvr_irq: PVR_TA_OPB_POS: %08lx\n",
            PVR_GET(PVR_TA_OPB_START), PVR_GET(PVR_TA_OPB_END), PVR_GET(PVR_TA_OPB_POS) << 2);
            break;

        case ASIC_EVT_PVR_TA_INPUT_ERR:
            dbglog(DBG_SOURCE(PVR_RENDER_DBG), "pvr_irq: ASIC_EVT_PVR_TA_INPUT_ERR\n");
            break;

        case ASIC_EVT_PVR_TA_INPUT_OVERFLOW:
            dbglog(DBG_SOURCE(PVR_RENDER_DBG), "pvr_irq: ASIC_EVT_PVR_TA_INPUT_OVERFLOW\n");
            break;
    }

    /* Update our stats if we finished all registration */
    switch(code) {
        case ASIC_EVT_PVR_OPAQUEDONE:
        case ASIC_EVT_PVR_TRANSDONE:
        case ASIC_EVT_PVR_OPAQUEMODDONE:
        case ASIC_EVT_PVR_TRANSMODDONE:
        case ASIC_EVT_PVR_PTDONE:

            if(pvr_state.lists_transferred != pvr_state.lists_enabled)
                return;

            pvr_sync_stats(PVR_SYNC_REGDONE);
            break;
    }

    // If all lists are fully transferred and a render is not in progress,
    // we are ready to start rendering.
    pvr_render_lists();
}
