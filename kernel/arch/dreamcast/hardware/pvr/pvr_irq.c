/* KallistiOS ##version##

   pvr_irq.c
   Copyright (C)2002,2004 Megan Potter

 */

#include <assert.h>
#include <dc/pvr.h>
#include <dc/asic.h>
#include <arch/cache.h>
#include "pvr_internal.h"

#include <kos/genwait.h>

#ifdef PVR_RENDER_DBG
#include <stdio.h>
#endif

/*
   PVR interrupt handler; the way things are setup, we're gonna get
   one of these for each full vertical refresh and at the completion
   of TA data acceptance. The timing here is pretty critical. We need
   to flip pages during a vertical blank, and then signal to the program
   that it's ok to start playing with TA registers again, or we waste
   rendering time.
*/

// Find the next list to DMA out. If we have none left to do, then do
// nothing. Otherwise, start the DMA and chain back to us upon completion.
static void dma_next_list(void *data) {
    int i, did = 0;
    volatile pvr_dma_buffers_t * b;

    (void)data;

    // DBG(("dma_next_list\n"));

    for(i = 0; i < PVR_OPB_COUNT; i++) {
        if((pvr_state.lists_enabled & (1 << i))
                && !(pvr_state.lists_dmaed & (1 << i))) {
            // Get the buffers for this frame.
            b = pvr_state.dma_buffers + (pvr_state.ram_target ^ 1);

            /* If we are in PVR DMA mode, yet we haven't associated a
               RAM-residing vertex buffer with the current list
               (because we submitted it directly, for example),
               mark it as complete, so we skip trying to DMA it. */
            if(!b->base[i]) {
                pvr_state.lists_dmaed       |= 1 << i;
                continue;
            }

            // Flush the last 32 bytes out of dcache, just in case.
            // dcache_flush_range((ptr_t)(b->base[i] + b->ptr[i] - 32), 32);
            dcache_flush_range((ptr_t)(b->base[i]), b->ptr[i] + 32);
            //amt = b->ptr[i] > 16384 ? 16384 : b->ptr[i];
            //dcache_flush_range((ptr_t)(b->base[i] + b->ptr[i] - amt), amt);

            // Start the DMA transfer, chaining to ourselves.
            //DBG(("dma_begin(buf %d, list %d, base %p, len %d)\n",
            //  pvr_state.ram_target ^ 1, i,
            //  b->base[i], b->ptr[i]));
            pvr_dma_load_ta(b->base[i], b->ptr[i], 0, dma_next_list, 0);

            // Mark this list as done, and break out for now.
            pvr_state.lists_dmaed |= 1 << i;
            did++;

            break;
        }
    }

    // If that was the last one, then free up the DMA channel.
    if(!did) {
        //DBG(("dma_complete(buf %d)\n", pvr_state.ram_target ^ 1));

        // If that was the last one, then free up the DMA channel.
        pvr_state.lists_dmaed = 0;

        // Unlock
        mutex_unlock((mutex_t *)&pvr_state.dma_lock);

        // Buffers are now empty again
        pvr_state.dma_buffers[pvr_state.ram_target ^ 1].ready = 0;
    }
}

void pvr_start_dma(void) {
    pvr_sync_stats(PVR_SYNC_REGSTART);

    mutex_lock((mutex_t *)&pvr_state.dma_lock);

    // Begin DMAing the first list.
    dma_next_list(0);
}

static void pvr_render_lists(void) {
    int bufn = pvr_state.view_target ^ 1;

    if(pvr_state.ta_busy
       && !pvr_state.render_busy
       && (!pvr_state.render_completed || pvr_state.to_texture[bufn])
       && pvr_state.lists_transferred == pvr_state.lists_enabled) {

        /* XXX Note:
           For some reason, the render must be started _before_ we sync
           to the new reg buffers. The only reasons I can think of for this
           are that there may be something in the reg sync that messes up
           the render in progress, or we are misusing some bits somewhere. */

        // Begin rendering from the dirty TA buffer into the clean
        // frame buffer.
        //DBG(("start_render(%d -> %d)\n", pvr_state.ta_target, pvr_state.view_target ^ 1));
        pvr_state.ta_target ^= pvr_state.vbuf_doublebuf;
        pvr_begin_queued_render();
        pvr_state.render_busy = 1;
        pvr_sync_stats(PVR_SYNC_RNDSTART);

        // Clear the texture render flag if we had it set.
        pvr_state.to_texture[bufn] = 0;

        // Switch to the clean TA buffer.
        pvr_state.lists_transferred = 0;
        pvr_sync_reg_buffer();

        // The TA is no longer busy.
        pvr_state.ta_busy = 0;

        // Signal the client code to continue onwards.
        genwait_wake_all((void *)&pvr_state.ta_busy);
        thd_schedule(1, 0);
    }
}

void pvr_vblank_handler(uint32 code, void *data) {
    (void)code;
    (void)data;

    pvr_sync_stats(PVR_SYNC_VBLANK);

    // If the render-done interrupt has fired then we are ready to flip to the
    // new frame buffer.
    if(pvr_state.render_completed) {
        int bufn = pvr_state.view_target;

        //DBG(("view(%d)\n", pvr_state.view_target ^ 1));

        // Handle PVR stats
        pvr_sync_stats(PVR_SYNC_PAGEFLIP);

        // Switch view address to the "good" buffer
        pvr_state.view_target ^= 1;

        if(!pvr_state.to_texture[bufn])
            pvr_sync_view();

        // Clear the render completed flag.
        pvr_state.render_completed = 0;
    }

    // We may have a pending render, that couldn't be done as the previous
    // render wasn't flipped yet; do it now.
    pvr_render_lists();
}

void pvr_int_handler(uint32 code, void *data) {
    (void)data;

    // What kind of event did we get?
    switch(code) {
        case ASIC_EVT_PVR_OPAQUEDONE:
            //DBG(("irq_opaquedone\n"));
            pvr_state.lists_transferred |= 1 << PVR_OPB_OP;
            break;
        case ASIC_EVT_PVR_TRANSDONE:
            //DBG(("irq_transdone\n"));
            pvr_state.lists_transferred |= 1 << PVR_OPB_TP;
            break;
        case ASIC_EVT_PVR_OPAQUEMODDONE:
            pvr_state.lists_transferred |= 1 << PVR_OPB_OM;
            break;
        case ASIC_EVT_PVR_TRANSMODDONE:
            pvr_state.lists_transferred |= 1 << PVR_OPB_TM;
            break;
        case ASIC_EVT_PVR_PTDONE:
            pvr_state.lists_transferred |= 1 << PVR_OPB_PT;
            break;
        case ASIC_EVT_PVR_RENDERDONE_TSP:
            //DBG(("irq_renderdone\n"));
            pvr_state.render_busy = 0;
            pvr_state.render_completed = 1;
            pvr_sync_stats(PVR_SYNC_RNDDONE);

            genwait_wake_all((void *)&pvr_state.render_busy);
            break;
    }

#ifdef PVR_RENDER_DBG
    /* Show register values on each interrupt */
    switch (code) {
        case ASIC_EVT_PVR_ISP_OUTOFMEM:
            DBG(("[ERROR]: ASIC_EVT_PVR_ISP_OUTOFMEM\n"));
            break;

        case ASIC_EVT_PVR_STRIP_HALT:
            DBG(("[ERROR]: ASIC_EVT_PVR_STRIP_HALT\n"));
            break;

        case ASIC_EVT_PVR_OPB_OUTOFMEM:
            DBG(("[ERROR]: ASIC_EVT_PVR_OPB_OUTOFMEM\n"));
            DBG(("PVR_TA_OPB_START: %08lx\nPVR_TA_OPB_END: %08lx\nPVR_TA_OPB_POS: %08lx\n",
                PVR_GET(PVR_TA_OPB_START),
                PVR_GET(PVR_TA_OPB_END),
                PVR_GET(PVR_TA_OPB_POS) << 2));
            break;

        case ASIC_EVT_PVR_TA_INPUT_ERR:
            DBG(("[ERROR]: ASIC_EVT_PVR_TA_INPUT_ERR\n"));
            break;

        case ASIC_EVT_PVR_TA_INPUT_OVERFLOW:
            DBG(("[ERROR]: ASIC_EVT_PVR_TA_INPUT_OVERFLOW\n"));
            break;
    }
#endif

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
