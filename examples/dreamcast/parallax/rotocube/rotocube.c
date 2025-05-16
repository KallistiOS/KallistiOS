/* KallistiOS ##version##

   rotocube.c
   Copyright (C)2003 Megan Potter
*/

#include <kos.h>
#include <math.h>
#include <plx/texture.h>
#include <plx/context.h>
#include <plx/prim.h>

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

/*

The fourth demo effect in my series of Parallax demo effects. A really
common demo effect back in the Amiga days (and somewhat in the middle
PC days) was "rotoscoping", where you have more or less silhouette
images of whatever you're animating. This example emulates that using
3D rendered objects.

Obviously what you can get away with drawing is somewhat limited to
things that the imagination can pick out of a silhouette but it's
neat and artistic looking.

The effect is so incredibly simple that some people probably wouldn't
even consider it a real effect, but it's one of those middle to later
PC demo day effects that is meant to be more artistic than impressive
code-wise. I went ahead and threw in a little wave generator to make
it more impressive looking.

One might also play with moving the wave to the front and switching it
to a translucent poly to interfere with the cube.

*/

static plx_dr_state_t dr;

// Draws the full cube at the current position.
void drawcube(void) {
    uint32_t color = 0xff808080;

    plx_dr_init(&dr);

    // Left face
    plx_vert_indm3(&dr, PLX_VERT, -3, -3, -3, color);
    plx_vert_indm3(&dr, PLX_VERT, -3,  3, -3, color);
    plx_vert_indm3(&dr, PLX_VERT, -3, -3,  3, color);
    plx_vert_indm3(&dr, PLX_VERT_EOS, -3,  3,  3, color);

    // Right face
    plx_vert_indm3(&dr, PLX_VERT, 3, -3, -3, color);
    plx_vert_indm3(&dr, PLX_VERT, 3,  3, -3, color);
    plx_vert_indm3(&dr, PLX_VERT, 3, -3,  3, color);
    plx_vert_indm3(&dr, PLX_VERT_EOS, 3,  3,  3, color);

    // Front face
    plx_vert_indm3(&dr, PLX_VERT, -3,  3, -3, color);
    plx_vert_indm3(&dr, PLX_VERT, -3, -3, -3, color);
    plx_vert_indm3(&dr, PLX_VERT,  3,  3, -3, color);
    plx_vert_indm3(&dr, PLX_VERT_EOS,  3, -3, -3, color);

    // Back face
    plx_vert_indm3(&dr, PLX_VERT, -3,  3, 3, color);
    plx_vert_indm3(&dr, PLX_VERT, -3, -3, 3, color);
    plx_vert_indm3(&dr, PLX_VERT,  3,  3, 3, color);
    plx_vert_indm3(&dr, PLX_VERT_EOS,  3, -3, 3, color);

    // Top face
    plx_vert_indm3(&dr, PLX_VERT, -3, -3, -3, color);
    plx_vert_indm3(&dr, PLX_VERT, -3, -3,  3, color);
    plx_vert_indm3(&dr, PLX_VERT,  3, -3, -3, color);
    plx_vert_indm3(&dr, PLX_VERT_EOS,  3, -3,  3, color);

    // Bottom face
    plx_vert_indm3(&dr, PLX_VERT, -3, 3, -3, color);
    plx_vert_indm3(&dr, PLX_VERT, -3, 3,  3, color);
    plx_vert_indm3(&dr, PLX_VERT,  3, 3, -3, color);
    plx_vert_indm3(&dr, PLX_VERT_EOS,  3, 3,  3, color);

    plx_dr_finish();
}

// Draws the wave in the background (2D)
void drawwave(int theta) {
    uint32_t color = 0xffa0a0a0;
    int i, divs = 320;
    float x, y, t;

    // Convert to radians for sin/cos
    t = theta * 2 * F_PI / 360.0f;

    for(i = 0; i <= divs; i++) {
        x = i * 640.0f / divs;

        // These are more or less magic numbers I played with until
        // it looked neat.
        y = 240.0f + fsin(t + i * F_PI / 64.0f) * 30.0f * fsin(t * 4);
        y += fcos(t + i * F_PI / 36.0f) * 40.0f * fcos(t * 6);
        y += fcos(t + i * F_PI / 30.0f) * 24.0f * fcos(t * 8);

        plx_vert_inp(PLX_VERT, x, y, 0.0001f, color);
        plx_vert_inp(i == divs ? PLX_VERT_EOS : PLX_VERT, x, 480.0f, 0.0001f, color);
    }
}

int main(int argc, char **argv) {
    int done;
    int theta;
    pvr_stats_t stats;

    // Init PVR
    pvr_init_defaults();
    pvr_set_bg_color(0.8f, 0.8f, 0.8f);

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

        // Draw a cube at our current position/rotation. We make a snapshot
        // of the full translation matrix after projection so that we can
        // just apply new modelview matrices for the delay cubes.
        plx_mat_identity();
        plx_mat3d_apply(PLX_MAT_SCREENVIEW);
        plx_mat3d_apply(PLX_MAT_PROJECTION);
        plx_mat3d_apply(PLX_MAT_MODELVIEW);
        drawcube();

        drawwave(theta);

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
