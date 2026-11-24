/* KallistiOS ##version##

   pvr_buffers.c
   Copyright (C) 2002, 2004 Megan Potter
   Copyright (C) 2014 Lawrence Sebald

 */

#include <assert.h>
#include <stdio.h>
#include <dc/pvr.h>
#include <dc/video.h>
#include <kos/regfield.h>

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

#define LIST_ENABLED(i) (pvr_state.lists_enabled & BIT(i))

size_t pvr_ra_write(uint32_t tile_matrix, int tw, int th,
                    const pvr_ra_group_t *groups, int group_count) {
    uint32_t *vr;   /* Note: We're working in 4-byte pointer maths here */
    const pvr_ra_group_t *group;
    int x, y, p, i, tn;

    vr = (uint32_t *)PVR_RAM_BASE + BYTES_TO_WORDS(tile_matrix - PVR_TILE_MATRIX_HEADER_SIZE);

    /*
        FIXME? Is this header necessary? If we're moving the tilematrix
        register to after it, how does the Dreamcast know this is here?
    */

    /* Header of zeros */
    for(i = 0; i < PVR_TILE_MATRIX_HEADER_SIZE; i += 4)
        *vr++ = 0;

    /* Initial init tile */
    vr[0] = PVR_RA_NO_WRITEOUT;
    vr[1] = PVR_RA_LIST_EMPTY;
    vr[2] = PVR_RA_LIST_EMPTY;
    vr[3] = PVR_RA_LIST_EMPTY;
    vr[4] = PVR_RA_LIST_EMPTY;
    vr[5] = PVR_RA_LIST_EMPTY;
    vr += PVR_RA_ENTRY_WORDS;

    /*
        This sets up the addresses for each list, for each tile, for each
        group. If a list isn't enabled, then we set the address to 0x80000000
        which tells the PVR to ignore it.

        Memory for each frame is arranged sort-of like this:

        [vertex_buffer | object pointer buffers | tilematrix header | tile matrix]

        The entries of a tile's groups follow each other, in group order.
    */
    for(x = 0; x < tw; x++) {
        for(y = 0; y < th; y++) {
            tn = (tw * y) + x;

            for(p = 0; p < group_count; p++) {
                group = groups + p;

                /* Control word */
                vr[0] = PVR_RA_TILE_Y(y) | PVR_RA_TILE_X(x)
                        | (group->flags & PVR_RA_FLAGS_MASK);

                /* Opaque poly, opaque volume mod, translucent poly,
                   translucent volume mod and punch-thru poly buffers */
                for(i = 0; i < PVR_OPB_COUNT; i++) {
                    if(group->opb[i] & PVR_RA_LIST_EMPTY)
                        vr[1 + i] = PVR_RA_LIST_EMPTY;
                    else
                        vr[1 + i] = group->opb[i] + (group->bin_size[i] * tn);
                }

                vr += PVR_RA_ENTRY_WORDS;
            }
        }
    }

    vr[-PVR_RA_ENTRY_WORDS] |= PVR_RA_LAST;

    return PVR_RA_SIZE(tw, th, group_count);
}

void pvr_ra_set_flags(uint32_t tile_matrix, int tw, int th, int group_count,
                      int p, uint32_t flags) {
    uint32_t *vr;
    int x, y;

    flags &= PVR_RA_FLAGS_MASK;

    /* Skip the dummy entry. Only control words are written, nothing is read
       back from VRAM. */
    vr = (uint32_t *)PVR_RAM_BASE + BYTES_TO_WORDS(tile_matrix)
         + PVR_RA_ENTRY_WORDS * (1 + p);

    for(x = 0; x < tw; x++) {
        for(y = 0; y < th; y++) {
            vr[0] = PVR_RA_TILE_Y(y) | PVR_RA_TILE_X(x) | flags;
            vr += PVR_RA_ENTRY_WORDS * group_count;
        }
    }

    /* The last tile's last group ends the array. */
    if(p == group_count - 1)
        vr[-PVR_RA_ENTRY_WORDS * group_count] |= PVR_RA_LAST;
}

/* Fill Tile Matrix buffers. This function takes a base address and sets up
   the rendering structures there. Each tile of the screen (32x32) receives
   a small buffer space. */
static void pvr_init_tile_matrix(int which, bool presort) {
    volatile pvr_ta_buffers_t *buf;
    pvr_ra_group_t group;
    int i;

    buf = pvr_state.ta_buffers + which;
    group.flags = presort ? PVR_RA_PRESORT : 0;

    for(i = 0; i < PVR_OPB_COUNT; i++) {
        group.bin_size[i] = pvr_state.opb_size[i];
        group.opb[i] = LIST_ENABLED(i) ? buf->opb_addresses[i] : PVR_RA_LIST_EMPTY;
    }

    pvr_ra_write(buf->tile_matrix, pvr_state.tw, pvr_state.th, &group, 1);
}

/* Fill all tile matrices */
void pvr_init_tile_matrices(bool presort) {
    int i;

    for(i = 0; i < 2; i++)
        pvr_init_tile_matrix(i, presort);
}

void pvr_set_presort_mode(bool presort) {
    uint32_t tile_matrix;

    if(__predict_false(!pvr_state.vbuf_doublebuf))
        pvr_wait_render_done();

    tile_matrix = pvr_state.ta_buffers[pvr_state.ta_target].tile_matrix;
    pvr_ra_set_flags(tile_matrix, pvr_state.tw, pvr_state.th, 1, 0,
                     presort ? PVR_RA_PRESORT : 0);
}


/* Allocate PVR buffers given a set of parameters

There's some confusion in here that is explained more fully in pvr_internal.h.

The other confusing thing is that texture ram is a 64-bit multiplexed space
rather than a copy of the flat 32-bit VRAM. So in order to maximize the
available texture RAM, the PVR structures for the two frames are broken
up and placed at 0x000000 and 0x400000.

*/
void pvr_allocate_buffers(const pvr_init_params_t *params) {
    volatile pvr_ta_buffers_t   *buf;
    volatile pvr_frame_buffers_t    *fbuf;
    int i, j;
    uint32_t  outaddr, sconst, opb_size_accum, opb_total_size;

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
    if(!__is_aligned(pvr_state.h, 32)) {
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

    /* Previously, we specified BIT(20) to say that the OPB grows "down" when
       the TA needs more than one. To make it grow "up" instead (increasing addresses),
       we set 0 as the default value.
     */
#if 0
    pvr_state.list_reg_mask = BIT(20);
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
            pvr_state.lists_enabled |= BIT(i);
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
        outaddr = __align_up(outaddr, 128);

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
        outaddr = __align_up(outaddr, 128);

        /* Tile Matrix header */
        outaddr += PVR_TILE_MATRIX_HEADER_SIZE;

        /* Tile Matrix */
        buf->tile_matrix = outaddr;
        buf->tile_matrix_size = PVR_RA_SIZE(pvr_state.tw, pvr_state.th, 1);
        outaddr += buf->tile_matrix_size;

        /* N-byte align */
        outaddr = __align_up(outaddr, 128);

        /* Output buffer */
        fbuf->frame = outaddr;
        fbuf->frame_size = pvr_state.w * pvr_state.h * vid_pmode_bpp[vid_mode->pm];
        outaddr += fbuf->frame_size;

        /* N-byte align */
        outaddr = __align_up(outaddr, 128);
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
    dbglog(DBG_KDEBUG, "  zclip %08lx\n", *((uint32_t *)&pvr_state.zclip));
    dbglog(DBG_KDEBUG, "  pclip_left/right %08lx/%08lx\n", pvr_state.pclip_left, pvr_state.pclip_right);
    dbglog(DBG_KDEBUG, "  pclip_top/bottom %08lx/%08lx\n", pvr_state.pclip_top, pvr_state.pclip_bottom);
    dbglog(DBG_KDEBUG, "  lists_enabled %08lx\n", pvr_state.lists_enabled);
    dbglog(DBG_KDEBUG, "Free texture memory: %ld bytes\n",
           0x800000 - pvr_state.texture_base);
#endif  /* !NDEBUG */
}
