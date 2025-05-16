/* KallistiOS ##version##

   delay_cube.c
   Copyright (C)2003 Megan Potter
*/

#include <kos.h>
#include <math.h>
#include <plx/texture.h>
#include <plx/context.h>
#include <plx/prim.h>

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

/*

The third demo effect in my series of Parallax demo effects. This one
might actually be of practical interest to people still. This effect
generates a cube out of a 3D grid of dots and then does something that
looks somewhat like motion blurring on it. The trick here is that we're
using alpha blended copies of the object instead of doing raster effects,
so it becomes feasible on the PVR.

This works by saving the last N modelview matrices and then restoring
each one for each "delay" cube. Each "delay" cube has a successively
lower alpha value as well.

This is very rendering stage intensive due to the alpha blending. It
might be made faster by switching entirely to opaque polys, but I'll
leave the working out of that as an exercise to the reader ;)

Another exercise for the reader... it is possible (and not too difficult)
to modify this code so that you have many geometric objects and morph
between them. The key is to have the same number of points in each object
and then have a "delta" array to tell you how much to move each point
each frame.

*/

// Generate a cube object for our usage. This could really be anything, I
// just like cubes. :)
vector_t cube[6 * 6 * 6];
int cubepnts = 6 * 6 * 6;
void makecube(void) {
    int x, y, z;

    for(x = -3; x < 3; x++) {
        for(y = -3; y < 3; y++) {
            for(z = -3; z < 3; z++) {
                cube[(x + 3) * 36 + (y + 3) * 6 + (z + 3)].x = x;
                cube[(x + 3) * 36 + (y + 3) * 6 + (z + 3)].y = y;
                cube[(x + 3) * 36 + (y + 3) * 6 + (z + 3)].z = z;
            }
        }
    }
}

// Draw a single point in 3D space. Uses the currently loaded matrix.
void drawpnt(float x, float y, float z, float a, float r, float g, float b) {
    uint32_t col = plx_pack_color(a, r, g, b);

    // Transform the point, clip the Z plane to avoid artifacts.
    plx_mat_tfip_3d(x, y, z);

    if(z <= 0.00001f) z = 0.00001f;

    // Draw it.
    plx_vert_inp(PLX_VERT, x, y + 1.0f, z, col);
    plx_vert_inp(PLX_VERT, x, y, z, col);
    plx_vert_inp(PLX_VERT, x + 1.0f, y + 1.0f, z, col);
    plx_vert_inp(PLX_VERT_EOS, x + 1.0f, y, z, col);
}

// Draws the full cube at the current position.
void drawcube(float b) {
    int i;

    for(i = 0; i < cubepnts; i++)
        drawpnt(cube[i].x, cube[i].y, cube[i].z, b, 1.0f, 1.0f, 1.0f);
}

// Temporary matrix so we don't have to keep recalculating our
// screenview and projection matrices. This has to be declared out
// here or GCC will ignore your aligned(32) request (potentially
// causing crashage).
static matrix_t tmpmat __attribute__((aligned(32)));

int main(int argc, char **argv) {
    int done, i;
    int theta;
    pvr_stats_t stats;
    int delays = 8;
    matrix_t oldmats[delays];

    memset(&oldmats, 0, sizeof(oldmats));

    // Init PVR
    pvr_init_defaults();
    pvr_set_bg_color(0.0f, 1.0f * 0x49 / 0xff, 1.0f * 0x6b / 0xff);

    // Setup the context
    plx_cxt_init();
    plx_cxt_texture(NULL);
    plx_cxt_culling(PLX_CULL_NONE);

    // Init 3D stuff. mat3d is like KGL.
    plx_mat3d_init();
    plx_mat3d_mode(PLX_MAT_PROJECTION);
    plx_mat3d_identity();
    plx_mat3d_perspective(45.0f, 640.0f / 480.0f, 0.1f, 100.0f);
    plx_mat3d_mode(PLX_MAT_MODELVIEW);

    // Create our model
    makecube();

    // Until the user hits start...
    for(done = 0, theta = 0; !done;) {
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

        // Setup some rotations/translations. These are "executed" from
        // bottom to top. So we rotate the cube in place on all three
        // axes a bit, then translate it to the left by 10, and rotate
        // the whole deal (so it zooms around the screen a bit). Finally
        // we translate the whole mess back by 30 so it's visible.
        plx_mat3d_identity();
        plx_mat3d_translate(0.0f, 0.0f, -30.0f);
        plx_mat3d_rotate(theta * 0.3f, 1.0f, 0.0f, 0.0f);
        plx_mat3d_rotate(theta * 1.5f, 0.0f, 1.0f, 0.0f);
        plx_mat3d_translate(10.0f, 0.0f, 0.0f);
        plx_mat3d_rotate(theta, 1.0f, 0.0f, 0.0f);
        plx_mat3d_rotate(theta * 0.5f, 0.0f, 0.1f, 0.0f);
        plx_mat3d_rotate(theta * 0.3f, 0.0f, 0.0f, 0.1f);

        // Save this latest modelview matrix
        for(i = 0; i < delays - 1; i++)
            memcpy(&oldmats[i], &oldmats[i + 1], sizeof(matrix_t));

        plx_mat3d_store(&oldmats[delays - 1]);

        // Draw a cube at our current position/rotation. We make a snapshot
        // of the full translation matrix after projection so that we can
        // just apply new modelview matrices for the delay cubes.
        plx_mat_identity();
        plx_mat3d_apply(PLX_MAT_SCREENVIEW);
        plx_mat3d_apply(PLX_MAT_PROJECTION);
        plx_mat_store(&tmpmat);
        plx_mat3d_apply(PLX_MAT_MODELVIEW);
        drawcube(1.0f);

        // Translucent list for delay cubes
        plx_list_begin(PVR_LIST_TR_POLY);
        plx_cxt_send(PVR_LIST_TR_POLY);

        // Now go back and draw the delay cubes
        for(i = 0; i < delays - 1; i++) {
            // Load the old matrix
            plx_mat3d_load(&oldmats[(delays - 2) - i]);

            // Apply it and draw the cube
            plx_mat_load(&tmpmat);
            plx_mat3d_apply(PLX_MAT_MODELVIEW);
            drawcube(1.0f - (i + 2) * (0.8f / delays));
        }

        pvr_scene_finish();

        // Move our counters
        theta++;
    }

    // You have to keep a watch on these, especially the vertex used
    // for really poly intensive effects.
    pvr_get_stats(&stats);
    dbglog(DBG_DEBUG, "3D Stats: %d vblanks, frame rate ~%f fps, max vertex used %d bytes\n",
           stats.vbl_count, (double)stats.frame_rate, stats.vtx_buffer_used_max);

    return 0;
}


