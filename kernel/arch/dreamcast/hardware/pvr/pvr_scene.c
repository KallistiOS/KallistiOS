/* KallistiOS ##version##

   pvr_scene.c
   Copyright (C)2002,2004 Megan Potter

 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <kos/thread.h>
#include <dc/pvr.h>
#include <dc/sq.h>
#include "pvr_internal.h"

/*

   Scene rendering

   Please see ../../include/dc/pvr.h for more info on this API!

*/

void * pvr_set_vertbuf(pvr_list_t list, void * buffer, int len) {
    void * oldbuf;

    // Make sure we have global DMA usage enabled. The DMA can still
    // be used in other situations, but the user must take care of
    // that themselves.
    assert(pvr_state.dma_mode);

    // Make sure it's a valid list.
    assert(list < PVR_OPB_COUNT);

    // Make sure it's an _enabled_ list.
    assert(pvr_state.lists_enabled & (1 << list));

    // Make sure the buffer parameters are valid.
    assert(!(((ptr_t)buffer) & 31));
    assert(!(len & 63));

    // Save the old value.
    oldbuf = pvr_state.dma_buffers[0].base[list];

    // Write new values.
    pvr_state.dma_buffers[0].base[list] = (uint8 *)buffer;
    pvr_state.dma_buffers[0].ptr[list] = 0;
    pvr_state.dma_buffers[0].size[list] = len / 2;
    pvr_state.dma_buffers[0].ready = 0;
    pvr_state.dma_buffers[1].base[list] = ((uint8 *)buffer) + len / 2;
    pvr_state.dma_buffers[1].ptr[list] = 0;
    pvr_state.dma_buffers[1].size[list] = len / 2;
    pvr_state.dma_buffers[1].ready = 0;

    return oldbuf;
}

void * pvr_vertbuf_tail(pvr_list_t list) {
    uint8 * bufbase;

    // Check the validity of the request.
    assert(list < PVR_OPB_COUNT);
    assert(pvr_state.dma_mode);

    // Get the buffer base.
    bufbase = pvr_state.dma_buffers[pvr_state.ram_target].base[list];
    assert(bufbase);

    // Return the current end of the buffer.
    return bufbase + pvr_state.dma_buffers[pvr_state.ram_target].ptr[list];
}

void pvr_vertbuf_written(pvr_list_t list, uint32 amt) {
    uint32 val;

    // Check the validity of the request.
    assert(list < PVR_OPB_COUNT);
    assert(pvr_state.dma_mode);

    // Change the current end of the buffer.
    val = pvr_state.dma_buffers[pvr_state.ram_target].ptr[list];
    val += amt;
    assert(val < pvr_state.dma_buffers[pvr_state.ram_target].size[list]);
    pvr_state.dma_buffers[pvr_state.ram_target].ptr[list] = val;
}

/* Begin collecting data for a frame of 3D output to the off-screen
   frame buffer */
void pvr_scene_begin(void) {
    int i;

    // Get general stuff ready.
    pvr_state.list_reg_open = -1;

    // Clear these out in case we're using DMA.
    if(pvr_state.dma_mode) {
        for(i = 0; i < PVR_OPB_COUNT; i++) {
            pvr_state.dma_buffers[pvr_state.ram_target].ptr[i] = 0;
        }

        pvr_sync_stats(PVR_SYNC_BUFSTART);
        // DBG(("pvr_scene_begin(dma -> %d)\n", pvr_state.ram_target));
    }
    else {
        pvr_state.lists_closed = 0;

        // We assume registration is starting immediately
        pvr_sync_stats(PVR_SYNC_REGSTART);
    }
}

/* Begin collecting data for a frame of 3D output to the specified texture;
   pass in the size of the buffer in rx and ry, and the return values in
   rx and ry will be the size actually used (if changed). Note that
   currently this only supports screen-sized output! */
/* Currently the resize functionality is not implemented, so make sure that
   rx and ry are appropriate (i.e. *rx = 1024 and *ry = 512 for 640x480).
   Also, note that this probably won't work with DMA mode for now... */
void pvr_scene_begin_txr(pvr_ptr_t txr, uint32 *rx, uint32 *ry) {
    int buf = pvr_state.view_target ^ 1;
    (void)ry;

    /* For the most part, this isn't very much different than the normal render setup.
       And, yes, if you remember KOS 1.1.6, this pretty much looks similar to what was
       there. I'm quite uncreative with my variable naming ;) */
    // Mark us as rendering to a texture
    pvr_state.to_texture[buf] = 1;

    // Set the render pitch up
    pvr_state.to_txr_rp[buf] = (*rx) * 2 / 8;

    // Set the output address
    pvr_state.to_txr_addr[buf] = (uint32)(txr) - PVR_RAM_INT_BASE;

    pvr_scene_begin();
}

/* Begin collecting data for the given list type. Lists do not have to be
   submitted in any particular order, but all types of a list must be
   submitted at once. If the given list has already been closed, then an
   error (-1) is returned. */
int pvr_list_begin(pvr_list_t list) {
    /* Check to make sure we can do this */
#ifndef NDEBUG
    if(!pvr_state.dma_mode && pvr_state.lists_closed & (1 << list)) {
        dbglog(DBG_WARNING, "pvr_list_begin: attempt to open already closed list\n");
        return -1;
    }

#endif  /* !NDEBUG */

    /* If we already had a list open, close it first */
    if(pvr_state.list_reg_open != -1 && pvr_state.list_reg_open != (int)list)
        pvr_list_finish();

    /* Ok, set the flag */
    pvr_state.list_reg_open = list;

    return 0;
}

/* End collecting data for the current list type. Lists can never be opened
   again within a single frame once they have been closed. Thus submitting
   a primitive that belongs in a closed list is considered an error. Closing
   a list that is already closed is also an error (-1). Note that if you open
   a list but do not submit any primitives, this causes a hardware error. For
   simplicity we just always submit a blank primitive. */
int pvr_list_finish(void) {
    /* Check to make sure we can do this */
#ifndef NDEBUG
    if(!pvr_state.dma_mode && pvr_state.list_reg_open == -1) {
        dbglog(DBG_WARNING, "pvr_list_finish: attempt to close unopened list\n");
        return -1;
    }

#endif  /* !NDEBUG */

    if(!pvr_state.dma_mode) {
        /* Release Store Queues if they are used */
        if(pvr_state.dr_used) {
            pvr_dr_finish();
        }

        /* In case we haven't sent anything in this list, send a dummy */
        pvr_blank_polyhdr(pvr_state.list_reg_open);

        /* Set the flags */
        pvr_state.lists_closed |= (1 << pvr_state.list_reg_open);

        /* Send an EOL marker */
        pvr_sq_set32((void *)0, 0, 32, PVR_DMA_TA);
    }

    pvr_state.list_reg_open = -1;

    return 0;
}

int pvr_prim(void * data, int size) {
    /* Check to make sure we can do this */
#ifndef NDEBUG
    if(pvr_state.list_reg_open == -1) {
        dbglog(DBG_WARNING, "pvr_prim: attempt to submit to unopened list\n");
        return -1;
    }

#endif  /* !NDEBUG */

    if(!pvr_state.dma_mode) {
        /* Send the data */
        pvr_sq_load((void *)0, data, size, PVR_DMA_TA);
    }
    else {
        return pvr_list_prim(pvr_state.list_reg_open, data, size);
    }

    return 0;
}

int pvr_list_prim(pvr_list_t list, void * data, int size) {
    volatile pvr_dma_buffers_t * b;

    b = pvr_state.dma_buffers + pvr_state.ram_target;
    assert(b->base[list]);

    assert(!(size & 31));

    memcpy(b->base[list] + b->ptr[list], data, size);
    b->ptr[list] += size;
    assert(b->ptr[list] <= b->size[list]);

    return 0;
}

void pvr_dr_init(pvr_dr_state_t *vtx_buf_ptr) {
    *vtx_buf_ptr = 0;
    sq_lock((void *)PVR_TA_INPUT);
    pvr_state.dr_used = 1;
}

void pvr_dr_finish(void) {
    pvr_state.dr_used = 0;
    sq_unlock();
}

int pvr_list_flush(pvr_list_t list) {
    (void)list;

    assert_msg(0, "not implemented yet");
    return -1;
}

/* Call this after you have finished submitting all data for a frame; once
   this has been called, you can not submit any more data until one of the
   pvr_scene_begin() functions is called again. An error (-1) is returned if
   you have not started a scene already. */
int pvr_scene_finish(void) {
    int i, o;
    volatile pvr_dma_buffers_t * b;

    /* Release Store Queues if they are used */
    if(pvr_state.dr_used) {
        pvr_dr_finish();
    }

    // If we're in DMA mode, then this works a little differently...
    if(pvr_state.dma_mode) {
        // DBG(("pvr_scene_finish(dma -> %d)\n", pvr_state.ram_target));
        // If any enabled lists are empty, fill them with a blank polyhdr. Also
        // add a zero-marker to the end of each list.
        b = pvr_state.dma_buffers + pvr_state.ram_target;

        for(i = 0; i < PVR_OPB_COUNT; i++) {
            // Not enabled -> skip
            if(!(pvr_state.lists_enabled & (1 << i)))
                continue;

            // Make sure there's at least one primitive in each.
            if(b->ptr[i] == 0) {
                pvr_blank_polyhdr_buf(i, (pvr_poly_hdr_t*)(b->base[i]));
                b->ptr[i] += 32;
            }

            // Put a zero-marker on the end.
            memset(b->base[i] + b->ptr[i], 0, 32);
            b->ptr[i] += 32;

            // Verify that there is no overrun.
            assert(b->ptr[i] <= b->size[i]);
        }

        // Flip buffers and mark them complete.
        o = irq_disable();
        pvr_state.dma_buffers[pvr_state.ram_target].ready = 1;
        pvr_state.ram_target ^= 1;
        irq_restore(o);

        pvr_sync_stats(PVR_SYNC_BUFDONE);
    }
    else {
        /* If a list was open, close it */
        if(pvr_state.list_reg_open != -1)
            pvr_list_finish();

        /* If any lists weren't submitted, then submit blank ones now */
        for(i = 0; i < PVR_OPB_COUNT; i++) {
            if((pvr_state.lists_enabled & (1 << i))
                    && (!(pvr_state.lists_closed & (1 << i)))) {
                pvr_list_begin(i);
                pvr_blank_polyhdr(i);
                pvr_list_finish();
            }
        }
    }

    /* Ok, now it's just a matter of waiting for the interrupt... */
    return 0;
}

int pvr_wait_ready(void) {
    int t;

    assert(pvr_state.valid);

    t = sem_wait_timed((semaphore_t *)&pvr_state.ready_sem, 100);

    if(t < 0) {
#if 0
        dbglog(DBG_WARNING, "pvr_wait_ready: timed out\n");
        printf("VERTBUF_ADDR: %08lx\n", PVR_GET(PVR_ISP_VERTBUF_ADDR));
        printf("TILEMAT_ADDR: %08lx\n", PVR_GET(PVR_ISP_TILEMAT_ADDR));
        printf("OPB_START: %08lx\n", PVR_GET(PVR_TA_OPB_START));
        printf("OPB_END: %08lx\n", PVR_GET(PVR_TA_OPB_END));
        printf("OPB_POS: %08lx\n", PVR_GET(PVR_TA_OPB_POS));
        printf("OPB_INIT: %08lx\n", PVR_GET(PVR_TA_OPB_INIT));
        printf("VERTBUF_START: %08lx\n", PVR_GET(PVR_TA_VERTBUF_START));
        printf("VERTBUF_END: %08lx\n", PVR_GET(PVR_TA_VERTBUF_END));
        printf("VERTBUF_POS: %08lx\n", PVR_GET(PVR_TA_VERTBUF_POS));
#endif
        return -1;
    }

    return 0;
}

int pvr_check_ready(void) {
    assert(pvr_state.valid);

    if(sem_count((semaphore_t *)&pvr_state.ready_sem) > 0)
        return 0;
    else
        return -1;
}
