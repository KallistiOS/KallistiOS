/* KallistiOS ##version##

   raster_melt.c
   Copyright (C)2003 Megan Potter
*/

#include <kos.h>
#include <png/png.h>
#include <plx/texture.h>
#include <plx/context.h>
#include <plx/prim.h>

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

/*

The PVR is not only good at 3D, it makes an excellent blitter and sprite
layering engine.

This simple Parallax example demonstrates a very old school demo effect --
the raster melt. The idea is to slowly draw a bitmap onto the screen one
scanline at a time, stretching out that one scanline to the bottom half of
the screen which hasn't been filled yet.

*/

int main(int argc, char **argv) {
    plx_texture_t * txr;
    int done;
    float y, xoffs;

    // Init PVR
    pvr_init_defaults();

    // Load a texture
    txr = plx_txr_load("/rd/megan.jpg", 0, 0);

    if(!txr)
        return 0;

    // Setup a texture context
    plx_cxt_init();
    plx_cxt_texture(txr);
    plx_cxt_culling(PLX_CULL_NONE);

    // The image will be offset by this much from each side
    xoffs = (640 - 512) / 2;

    // Until the user hits start...
    for(done = 0, y = 0.0f; !done;) {
        // Check for start
        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)

        if(st->buttons & CONT_START)
            done = 1;

        MAPLE_FOREACH_END()

        // Setup the frame
        pvr_wait_ready();
        pvr_scene_begin();
        pvr_list_begin(PVR_LIST_OP_POLY);

        // Submit the context
        plx_cxt_send(PVR_LIST_OP_POLY);

        // Two polys -- one normal up to the scan point, and one
        // stretched to the bottom.
        plx_vert_ifp(PLX_VERT, xoffs, y, 1.0f, 0xffffffff, 0.0f, y / 480.0f);
        plx_vert_ifp(PLX_VERT, xoffs, 0.0f, 1.0f, 0xffffffff, 0.0f, 0.0f);
        plx_vert_ifp(PLX_VERT, 640.0f - xoffs, y, 1.0f, 0xffffffff, 1.0f, y / 480.0f);
        plx_vert_ifp(PLX_VERT_EOS, 640.0f - xoffs, 0.0f, 1.0f, 0xffffffff, 1.0f, 0.0f);

        plx_vert_ifp(PLX_VERT, xoffs, 480.0f, 1.0f, 0xffffffff, 0.0f, y / 480.0f);
        plx_vert_ifp(PLX_VERT, xoffs, y, 1.0f, 0xffffffff, 0.0f, y / 480.0f);
        plx_vert_ifp(PLX_VERT, 640.0f - xoffs, 480.0f, 1.0f, 0xffffffff, 1.0f, y / 480.0f);
        plx_vert_ifp(PLX_VERT_EOS, 640.0f - xoffs, y, 1.0f, 0xffffffff, 1.0f, y / 480.0f);

        pvr_scene_finish();

        // Move our scanline
        if(y < 480.0f)
            y += 2.0f;
    }

    plx_txr_destroy(txr);
    return 0;
}


