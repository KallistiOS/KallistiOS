/* KallistiOS ##version##

   pvr_buffers.c
   Copyright (C) 2002, 2004 Megan Potter
   Copyright (C) 2014 Lawrence Sebald

 */

#include <assert.h>
#include <stdio.h>
#include <dc/pvr.h>
#include <dc/video.h>
#include "pvr_internal.h"

/*

  This module handles buffer allocation for the structures that the
  TA feed into, and which the ISP/TSP read from during the scene
  rendering.

*/


/* There's quite a bit of byte vs word conversion in this file
 * these macros just help make that more readable */
#define BYTES_TO_WORDS(x) ((x) >> 2)
#define WORDS_TO_BYTES(x) ((x) << 2)

#define IS_ALIGNED(x, m) ((x) % (m) == 0)

#define LIST_ENABLED(i) (pvr_state.lists_enabled & (1 << (i)))


/* Fill Tile Matrix buffers. This function takes a base address and sets up
   the rendering structures there. Each tile of the screen (32x32) receives
   a small buffer space. */
static void pvr_init_tile_matrix(int which, bool presort) {
    volatile pvr_ta_buffers_t   *buf;
    int     x, y, tn;
    uint32      *vr;  /* Note: We're working in 4-byte pointer maths in this function */
    volatile int    *opb_sizes;
    //uint32      matbase, opbbase;

    vr = (uint32*)PVR_RAM_BASE;
    buf = pvr_state.ta_buffers + which;
    opb_sizes = pvr_state.opb_size;

#if 0
    matbase = buf->tile_matrix;
    opbbase = buf->opb;
#endif

    /*
        FIXME? Is this header necessary? If we're moving the tilematrix
        register to after it, how does the Dreamcast know this is here?
    */

    /* Header of zeros */
    vr += BYTES_TO_WORDS(buf->tile_matrix);

    for(x = 0; x < 0x48; x += 4)
        * vr++ = 0;

    /* Initial init tile */
    vr[0] = 0x10000000;
    vr[1] = 0x80000000;
    vr[2] = 0x80000000;
    vr[3] = 0x80000000;
    vr[4] = 0x80000000;
    vr[5] = 0x80000000;
    vr += 6;

    /* Must skip over zeroed header for actual usage */
    buf->tile_matrix += 0x48;

    /* Now the main tile matrix */
#if 0
    dbglog(DBG_KDEBUG, "  Using poly buffers %08lx/%08lx/%08lx/%08lx/%08lx\r\n",
           buf->opb_type[0],
           buf->opb_type[1],
           buf->opb_type[2],
           buf->opb_type[3],
           buf->opb_type[4]);
#endif  /* !NDEBUG */

    /*
        This sets up the addresses for each list, for each tile in the
        memory we allocate in pvr_allocate_buffers. If a list isn't enabled
        for a tile, then we set the address to 0x80000000 which tells the PVR
        to ignore it.

        Memory for each frame is arranged sort-of like this:

        [vertex_buffer | object pointer buffers | tilematrix header | tile matrix]

        This is the tile matrix setup.
    */

    for(x = 0; x < pvr_state.tw; x++) {
        for(y = 0; y < pvr_state.th; y++) {
            tn = (pvr_state.tw * y) + x;

            /* Control word */
            vr[0] = (y << 8) | (x << 2) | (presort << 29);

            /* Opaque poly buffer */
            vr[1] = LIST_ENABLED(0) ? buf->opb_addresses[0] + (opb_sizes[0] * tn) : 0x80000000;

            /* Opaque volume mod buffer */
            vr[2] = LIST_ENABLED(1) ? buf->opb_addresses[1] + (opb_sizes[1] * tn) : 0x80000000;

            /* Translucent poly buffer */
            vr[3] = LIST_ENABLED(2) ? buf->opb_addresses[2] + (opb_sizes[2] * tn) : 0x80000000;

            /* Translucent volume mod buffer */
            vr[4] = LIST_ENABLED(3) ? buf->opb_addresses[3] + (opb_sizes[3] * tn) : 0x80000000;

            /* Punch-thru poly buffer */
            vr[5] = LIST_ENABLED(4) ? buf->opb_addresses[4] + (opb_sizes[4] * tn) : 0x80000000;
            vr += 6;
        }
    }

    vr[-6] |= 1 << 31;
}

/* Fill all tile matrices */
void pvr_init_tile_matrices(bool presort) {
    int i;

    for(i = 0; i < 2; i++)
        pvr_init_tile_matrix(i, presort);
}

void pvr_set_presort_mode(bool presort) {
    pvr_init_tile_matrix(pvr_state.ta_target, presort);
}


/* Allocate PVR buffers given a set of parameters

There's some confusion in here that is explained more fully in pvr_internal.h.

The other confusing thing is that texture ram is a 64-bit multiplexed space
rather than a copy of the flat 32-bit VRAM. So in order to maximize the
available texture RAM, the PVR structures for the two frames are broken
up and placed at 0x000000 and 0x400000.

*/
#define BUF_ALIGN 128
#define BUF_ALIGN_MASK (BUF_ALIGN - 1)
#define APPLY_ALIGNMENT(addr) (((addr) + BUF_ALIGN_MASK) & ~BUF_ALIGN_MASK)

void pvr_allocate_buffers(const pvr_init_params_t *params) {
    volatile pvr_ta_buffers_t   *buf;
    volatile pvr_frame_buffers_t    *fbuf;
    int i, j;
    uint32  outaddr, sconst, opb_size_accum, opb_total_size;

    /* Set screen sizes; pvr_init has ensured that we have a valid mode
       and all that by now, so we can freely dig into the vid_mode
       structure here. */
    pvr_state.w = vid_mode->width;
    pvr_state.h = vid_mode->height;
    pvr_state.tw = pvr_state.w / 32;
    pvr_state.th = pvr_state.h / 32;

    /* FSAA -> double the tile buffer width */
    if(pvr_state.fsaa)
        pvr_state.tw *= 2;

    /* We can actually handle non-mod-32 heights pretty easily -- just extend
       the frame buffer a bit, but use a pixel clip for the real mode. */
    if(!IS_ALIGNED(pvr_state.h, 32)) {
        pvr_state.h = (pvr_state.h + 32) & ~31;
        pvr_state.th++;
    }

    pvr_state.tsize_const = ((pvr_state.th - 1) << 16)
                            | ((pvr_state.tw - 1) << 0);

    /* Set clipping parameters */
    pvr_state.zclip = 0.0001f;
    pvr_state.pclip_left = 0;
    pvr_state.pclip_right = vid_mode->width - 1;
    pvr_state.pclip_top = 0;
    pvr_state.pclip_bottom = vid_mode->height - 1;
    pvr_state.pclip_x = (pvr_state.pclip_right << 16) | (pvr_state.pclip_left);
    pvr_state.pclip_y = (pvr_state.pclip_bottom << 16) | (pvr_state.pclip_top);

    /* Look at active lists and figure out how much to allocate
       for each poly type */
    opb_total_size = 0;

    /* Previously, we specified 1 << 20 to say that the OPB grows "down" when
       the TA needs more than one. To make it grow "up" instead (increasing addresses),
       we set 0 as the default value.
     */
#if 0
    pvr_state.list_reg_mask = 1 << 20;
#else
    pvr_state.list_reg_mask = 0;
#endif

    for(i = 0; i < PVR_OPB_COUNT; i++) {
        pvr_state.opb_size[i] = WORDS_TO_BYTES(params->opb_sizes[i]);   /* in bytes */

        /* Calculate the total size of the OPBs for this list */
        opb_total_size += pvr_state.opb_size[i] * pvr_state.tw * pvr_state.th;

        switch(params->opb_sizes[i]) {
            case PVR_BINSIZE_0:
                sconst = 0;
                break;
            case PVR_BINSIZE_8:
                sconst = 1;
                break;
            case PVR_BINSIZE_16:
                sconst = 2;
                break;
            case PVR_BINSIZE_32:
                sconst = 3;
                break;
            default:
                assert_msg(0, "invalid poly_buf_size");
                sconst = 2;
                break;
        }

        if(sconst > 0) {
            pvr_state.lists_enabled |= (1 << i);
            pvr_state.list_reg_mask |= sconst << (4 * i);
        }
    }

    /* Initialize each buffer set */
    for(i = 0; i < 2; i++) {
        /* Frame 0 goes at 0, Frame 1 goes at 0x400000 (half way) */
        if(i == 0)
            outaddr = 0;
        else
            outaddr = 0x400000;

        /* Select a pvr_buffers_t. Note that there's no good reason
           to allocate the frame buffers at the same time as the TA
           buffers except that it's handy to do it all in one place. */
        buf = pvr_state.ta_buffers + i;
        fbuf = pvr_state.frame_buffers + i;

        /* Vertex buffer */
        buf->vertex = outaddr;
        buf->vertex_size = params->vertex_buf_size;
        outaddr += buf->vertex_size;
        /* N-byte align */
        outaddr = APPLY_ALIGNMENT(outaddr);

        /* Object Pointer Blocks */
        buf->opb = outaddr;
        buf->opb_size = opb_total_size;

        /* Allocate extra space for overflow (when one OPB isn't big enough) */
        buf->opb_overflow_count = params->opb_overflow_count;
        outaddr += opb_total_size * (1 + buf->opb_overflow_count);

        /* Set up the opb pointers to each section */
        opb_size_accum = 0;
        for(j = 0; j < PVR_OPB_COUNT; j++) {
            buf->opb_addresses[j] = buf->opb + opb_size_accum;
            opb_size_accum += pvr_state.opb_size[j] * pvr_state.tw * pvr_state.th;
        }

        assert(buf->opb_size == opb_size_accum);

        /* N-byte align */
        outaddr = APPLY_ALIGNMENT(outaddr);

        /* Tile Matrix */
        buf->tile_matrix = outaddr;
        buf->tile_matrix_size = WORDS_TO_BYTES(18 + 6 * pvr_state.tw * pvr_state.th);
        outaddr += buf->tile_matrix_size;

        /* N-byte align */
        outaddr = APPLY_ALIGNMENT(outaddr);

        /* Output buffer */
        fbuf->frame = outaddr;
        fbuf->frame_size = pvr_state.w * pvr_state.h * vid_pmode_bpp[vid_mode->pm];
        outaddr += fbuf->frame_size;

        /* N-byte align */
        outaddr = APPLY_ALIGNMENT(outaddr);
    }

    /* Texture ram is whatever is left */
    pvr_state.texture_base = (outaddr - 0x400000) * 2;

#if 0
    dbglog(DBG_KDEBUG, "pvr: initialized PVR buffers:\n");
    dbglog(DBG_KDEBUG, "  texture RAM begins at %08lx\n", pvr_state.texture_base);

    for(i = 0; i < 2; i++) {
        buf = pvr_state.ta_buffers + i;
        fbuf = pvr_state.frame_buffers + i;
        dbglog(DBG_KDEBUG, "  vertex/vertex_size: %08lx/%08lx\n", buf->vertex, buf->vertex_size);
        dbglog(DBG_KDEBUG, "  opb base/opb_size: %08lx/%08lx\n", buf->opb, buf->opb_size);
        dbglog(DBG_KDEBUG, "  opbs per type: %08lx %08lx %08lx %08lx %08lx\n",
               buf->opb_type[0],
               buf->opb_type[1],
               buf->opb_type[2],
               buf->opb_type[3],
               buf->opb_type[4]);
        dbglog(DBG_KDEBUG, "  tile_matrix/tile_matrix_size: %08lx/%08lx\n", buf->tile_matrix, buf->tile_matrix_size);
        dbglog(DBG_KDEBUG, "  frame/frame_size: %08lx/%08lx\n", fbuf->frame, fbuf->frame_size);
    }

    dbglog(DBG_KDEBUG, "  list_mask %08lx\n", pvr_state.list_reg_mask);
    dbglog(DBG_KDEBUG, "  w/h = %d/%d, tw/th = %d/%d\n", pvr_state.w, pvr_state.h,
           pvr_state.tw, pvr_state.th);
    dbglog(DBG_KDEBUG, "  zclip %08lx\n", *((uint32*)&pvr_state.zclip));
    dbglog(DBG_KDEBUG, "  pclip_left/right %08lx/%08lx\n", pvr_state.pclip_left, pvr_state.pclip_right);
    dbglog(DBG_KDEBUG, "  pclip_top/bottom %08lx/%08lx\n", pvr_state.pclip_top, pvr_state.pclip_bottom);
    dbglog(DBG_KDEBUG, "  lists_enabled %08lx\n", pvr_state.lists_enabled);
    dbglog(DBG_KDEBUG, "Free texture memory: %ld bytes\n",
           0x800000 - pvr_state.texture_base);
#endif  /* !NDEBUG */
}
