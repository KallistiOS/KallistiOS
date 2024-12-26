/* KallistiOS ##version##

   pvr_init_shutdown.c
   Copyright (C) 2002, 2004 Megan Potter

 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <dc/pvr.h>
#include <dc/video.h>
#include <dc/asic.h>
#include <dc/vblank.h>
#include "pvr_internal.h"

/*

   Initialization and shutdown: stuff you should only ever have to do
   once in your program.

*/

/* Simpler function which initializes the PVR using 16/16 for the opaque
   and translucent lists, and 0's for everything else; 512k of vertex
   buffer. This is equivalent to the old ta_init_defaults() for now. */
int pvr_init_defaults(void) {
    pvr_init_params_t params = {
        /* Enable opaque and translucent polygons with size 16 */
        { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0 },

        /* Vertex buffer size 512K */
        512 * 1024,

        /* No DMA */
        0,

        /* No FSAA */
        0,

        /* Translucent Autosort enabled. */
        0,

        /* Extra OPBs */
        3,

        /* Vertex buffer double-buffering enabled */
        0
    };

    return pvr_init(&params);
}

/* Initialize the PVR chip to ready status, enabling the specified lists
   and using the specified parameters; note that bins and vertex buffers
   come from the texture memory pool! Expects that a 2D mode was
   initialized already using the vid_* API. */
int pvr_init(pvr_init_params_t *params) {
    /* If we're already initialized, fail */
    if(pvr_state.valid == 1) {
        dbglog(DBG_WARNING, "pvr: pvr_init called twice!\n");
        return -1;
    }

    /* Make sure we got valid parameters */
    assert(params != NULL);

    /* Make sure that a video mode has been initialized */
    assert(vid_mode != NULL);
    assert(vid_mode->width != 0 && vid_mode->height != 0);

    /* Check for compatibility with 3D stuff */
    if((vid_mode->width % 32) != 0) {
        dbglog(DBG_WARNING, "pvr: mode %dx%d isn't usable for 3D (width not multiples of 32)\n",
               vid_mode->width, vid_mode->height);
        return -1;
    }

    /* Clear out video memory */
    vid_empty();

    /* Reset all PVR systems (in case it's still doing something) */
    PVR_SET(PVR_RESET, PVR_RESET_ALL);
    PVR_SET(PVR_RESET, PVR_RESET_NONE);

    /* Start off with a nice empty structure */
    memset((void *)&pvr_state, 0, sizeof(pvr_state));

    // Enable DMA if the user wants that.
    pvr_state.dma_mode = params->dma_enabled;

    // Copy over FSAA setting.
    pvr_state.fsaa = params->fsaa_enabled;

    pvr_state.vbuf_doublebuf = !params->vbuf_doublebuf_disabled;

    /* Everything's clear, do the initial buffer pointer setup */
    pvr_allocate_buffers(params);

    // Initialize tile matrices
    pvr_init_tile_matrices(!!params->autosort_disabled);

    // Setup all pipeline targets. Yes, this is redundant. :) I just
    // like to have it explicit.
    pvr_state.ram_target = 0;
    pvr_state.ta_target = 0;
    pvr_state.view_target = 0;

    pvr_state.list_reg_open = -1;

    // Sync all the hardware registers with our pipeline state.
    pvr_sync_view();
    pvr_sync_reg_buffer();

    // Clear out our stats
    pvr_state.vbl_count = 0;
    pvr_state.frame_last_time = 0;
    pvr_state.buf_start_time = 0;
    pvr_state.reg_start_time = 0;
    pvr_state.rnd_start_time = 0;
    pvr_state.frame_last_len = -1;
    pvr_state.buf_last_len = -1;
    pvr_state.reg_last_len = -1;
    pvr_state.rnd_last_len = -1;
    pvr_state.vtx_buf_used = 0;
    pvr_state.vtx_buf_used_max = 0;
    pvr_state.dr_used = 0;

    /* If we're on a VGA box, disable vertical smoothing */
    if(vid_mode->cable_type == CT_VGA) {
        dbglog(DBG_KDEBUG, "pvr: disabling vertical scaling for VGA\n");

        if(pvr_state.fsaa)
            PVR_SET(PVR_SCALER_CFG, 0x10400);
        else
            PVR_SET(PVR_SCALER_CFG, 0x400);
    }
    else {
        dbglog(DBG_KDEBUG, "pvr: enabling vertical scaling for non-VGA\n");

        if(pvr_state.fsaa)
            PVR_SET(PVR_SCALER_CFG, 0x10401);
        else
            PVR_SET(PVR_SCALER_CFG, 0x401);
    }

    /* Hook the PVR interrupt events on G2 */
    pvr_state.vbl_handle = vblank_handler_add(pvr_vblank_handler, NULL);
    
    asic_evt_set_handler(ASIC_EVT_PVR_OPAQUEDONE, pvr_int_handler, NULL);
    asic_evt_enable(ASIC_EVT_PVR_OPAQUEDONE, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_OPAQUEMODDONE, pvr_int_handler, NULL);
    asic_evt_enable(ASIC_EVT_PVR_OPAQUEMODDONE, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_TRANSDONE, pvr_int_handler, NULL);
    asic_evt_enable(ASIC_EVT_PVR_TRANSDONE, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_TRANSMODDONE, pvr_int_handler, NULL);
    asic_evt_enable(ASIC_EVT_PVR_TRANSMODDONE, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_PTDONE, pvr_int_handler, NULL);
    asic_evt_enable(ASIC_EVT_PVR_PTDONE, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_RENDERDONE_TSP, pvr_int_handler, NULL);
    asic_evt_enable(ASIC_EVT_PVR_RENDERDONE_TSP, ASIC_IRQ_DEFAULT);

#ifdef PVR_RENDER_DBG
    /* Hook up interrupt handlers for error events */
    asic_evt_set_handler(ASIC_EVT_PVR_ISP_OUTOFMEM, pvr_int_handler, NULL);
    asic_evt_enable(ASIC_EVT_PVR_ISP_OUTOFMEM, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_STRIP_HALT, pvr_int_handler, NULL);
    asic_evt_enable(ASIC_EVT_PVR_STRIP_HALT, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_OPB_OUTOFMEM, pvr_int_handler, NULL);
    asic_evt_enable(ASIC_EVT_PVR_OPB_OUTOFMEM, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_TA_INPUT_ERR, pvr_int_handler, NULL);
    asic_evt_enable(ASIC_EVT_PVR_TA_INPUT_ERR, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_TA_INPUT_OVERFLOW, pvr_int_handler, NULL);
    asic_evt_enable(ASIC_EVT_PVR_TA_INPUT_OVERFLOW, ASIC_IRQ_DEFAULT);
#endif

    /* 3d-specific parameters; these are all about rendering and
       nothing to do with setting up the video; some stuff in here
       is still unknown. */
    PVR_SET(PVR_UNK_00A8, 0x15d1c951);      /* M (Unknown magic value) */
    PVR_SET(PVR_UNK_00A0, 0x00000020);      /* M */
    /* PVR_FB_CFG_2 is configured in vid_set_mode() */
    PVR_SET(PVR_UNK_0110, 0x00093f39);      /* M */
    PVR_SET(PVR_UNK_0098, 0x00800408);      /* M */
    PVR_SET(PVR_TEXTURE_CLIP, 0x00000000);      /* texture clip distance */
    PVR_SET(PVR_SPANSORT_CFG, 0x00000101);      /* M */
    PVR_SET(PVR_FOG_TABLE_COLOR, 0x007f7f7f);   /* Fog table color */
    PVR_SET(PVR_FOG_VERTEX_COLOR, 0x007f7f7f);  /* Fog vertex color */
    PVR_SET(PVR_COLOR_CLAMP_MIN, 0x00000000);   /* color clamp min */
    PVR_SET(PVR_COLOR_CLAMP_MAX, 0xffffffff);   /* color clamp max */
    PVR_SET(PVR_UNK_0080, 0x00000007);      /* M */
    PVR_SET(PVR_CHEAP_SHADOW, 0x00000001);      /* cheap shadow */
    PVR_SET(PVR_UNK_007C, 0x0027df77);      /* M */
    PVR_SET(PVR_TEXTURE_MODULO, 0x00000000);    /* stride width */
    PVR_SET(PVR_FOG_DENSITY, 0x0000ff07);       /* fog density */
    PVR_SET(PVR_UNK_0118, 0x00008040);      /* M */

    /* Initialize PVR DMA */
    mutex_init((mutex_t *)&pvr_state.dma_lock, MUTEX_TYPE_NORMAL);
    pvr_dma_init();

    /* Set us as valid and return success */
    pvr_state.valid = 1;

    /* Validate our memory pool */
    pvr_mem_reset();
    /* This doesn't work right now... */
    /*#ifndef NDEBUG
        dbglog(DBG_KDEBUG, "pvr: free memory is %08lx bytes\n",
            pvr_mem_available());
    #endif*//* !NDEBUG */

    return 0;
}

/* Shut down the PVR chip from ready status, leaving it in 2D mode as it
   was before the init. */
int pvr_shutdown(void) {
    if(!pvr_state.valid)
        return -1;

    /* Set us invalid */
    pvr_state.valid = 0;

    /* Stop anything that might be going on */
    PVR_SET(PVR_RESET, PVR_RESET_ALL);
    PVR_SET(PVR_RESET, PVR_RESET_NONE);

    /* Unhook any int handlers */
    vblank_handler_remove(pvr_state.vbl_handle);
    asic_evt_remove_handler(ASIC_EVT_PVR_OPAQUEDONE);
    asic_evt_disable(ASIC_EVT_PVR_OPAQUEDONE, ASIC_IRQ_DEFAULT);
    asic_evt_remove_handler(ASIC_EVT_PVR_OPAQUEMODDONE);
    asic_evt_disable(ASIC_EVT_PVR_OPAQUEMODDONE, ASIC_IRQ_DEFAULT);
    asic_evt_remove_handler(ASIC_EVT_PVR_TRANSDONE);
    asic_evt_disable(ASIC_EVT_PVR_TRANSDONE, ASIC_IRQ_DEFAULT);
    asic_evt_remove_handler(ASIC_EVT_PVR_TRANSMODDONE);
    asic_evt_disable(ASIC_EVT_PVR_TRANSMODDONE, ASIC_IRQ_DEFAULT);
    asic_evt_remove_handler(ASIC_EVT_PVR_PTDONE);
    asic_evt_disable(ASIC_EVT_PVR_PTDONE, ASIC_IRQ_DEFAULT);
    asic_evt_remove_handler(ASIC_EVT_PVR_RENDERDONE_TSP);
    asic_evt_disable(ASIC_EVT_PVR_RENDERDONE_TSP, ASIC_IRQ_DEFAULT);

    /* Shut down PVR DMA */
    pvr_dma_shutdown();

    /* Invalidate our memory pool */
    pvr_mem_reset();

    /* Destroy the mutex */
    mutex_destroy((mutex_t *)&pvr_state.dma_lock);

    /* Clear video memory */
    vid_empty();

    /* Reset the frame buffer offset */
    vid_waitvbl();
    vid_set_start(0);

    /* Return success */
    return 0;
}
