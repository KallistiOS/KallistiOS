/* KallistiOS ##version##

   sinus.c
   Copyright (C)2003 Megan Potter
*/

#include <kos.h>
#include <math.h>
#include <plx/texture.h>
#include <plx/context.h>
#include <plx/prim.h>

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

/*

Another little old school demo effect from the Amiga, sinus bars (sometimes
called Copper bars, because they were done with the Copper chip on the
Amiga). Back in the Amiga days they had to program the Copper to change
a palette entry for each scanline at horizontal refresh. We don't have
it so hard these days with the PVR. In fact a lot of people would probably
consider this cheating ;)

*/

// This will be two polygons, each being half a gradient bar. Note that
// it's ok to mix vertex macro types.
void drawbar(float y, float z, float r, float g, float b) {
    // Top poly (black to white)
    plx_vert_fnp(PLX_VERT, 0.0f, y, z, 1.0f, r, g, b);
    plx_vert_inp(PLX_VERT, 0.0f, y - 20.0f, z, 0xff000000);
    plx_vert_fnp(PLX_VERT, 640.0f, y, z, 1.0f, r, g, b);
    plx_vert_inp(PLX_VERT_EOS, 640.0f, y - 20, z, 0xff000000);

    // Bottom poly (white to black)
    plx_vert_inp(PLX_VERT, 0.0f, y + 20.0f, z, 0xff000000);
    plx_vert_fnp(PLX_VERT, 0.0f, y, z, 1.0f, r, g, b);
    plx_vert_inp(PLX_VERT, 640.0f, y + 20.0f, z, 0xff000000);
    plx_vert_fnp(PLX_VERT_EOS, 640.0f, y, z, 1.0f, r, g, b);
}

int main(int argc, char **argv) {
    int done, i;
    float theta, dt;
    float colors[3 * 10] = {
        0.0f, 0.5f, 1.0f,
        0.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        0.5f, 0.5f, 0.0f,
        0.5f, 0.0f, 0.5f,
        0.5f, 0.5f, 0.5f,
        0.73f, 0.8f, 0.25f,
        0.25f, 0.8f, 0.73f,
        1.0f, 1.0f, 0.0f
    };

    // Init PVR
    pvr_init_defaults();

    // Setup the context
    plx_cxt_init();
    plx_cxt_texture(NULL);
    plx_cxt_culling(PLX_CULL_NONE);

    // Until the user hits start...
    dt = 2 * F_PI / 160.0f;

    for(done = 0, theta = 0.0f; !done;) {
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

        // Draw a sinus bar at our current position and several positions
        // back. Each bar will get its own Z value (descending).
        for(i = 0; i < 10; i++) {
            drawbar(240.0f + fsin(theta - dt * i * 6) * 120.0f, 100.0f - i,
                    colors[i * 3 + 0], colors[i * 3 + 1], colors[i * 3 + 2]);
        }

        pvr_scene_finish();

        // Move our counters
        theta += dt;

        while(theta >= 2 * F_PI)
            theta -= 2 * F_PI;
    }

    return 0;
}


