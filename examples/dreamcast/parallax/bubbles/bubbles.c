/*
   KallistiOS ##version##
   bubbles.c

   (c)2002 Megan Potter
*/

#include <kos.h>
#include <math.h>
#include <plx/matrix.h>
#include <plx/prim.h>

/*

This is a Parallax version of the "bubbles" KGL demo. The speed is about
the same, but it uses the Parallax functions instead of KGL.

*/

/* Draws a sphere out of GL_QUADS according to the syntax of glutSolidSphere

   This now uses triangle strips and PVR Direct Rendering for the best
   possible performance.

   Based very loosely on a routine from James Susinno.
*/
static float phase = 0.0f;
static pvr_poly_cxt_t cxt;
static pvr_poly_hdr_t hdr;
static plx_dr_state_t  dr_state;
static void sphere(float radius, int slices, int stacks) {
    int i, j;
    float   pitch, pitch2;
    float   x, y, z, g, b;
    float   yaw;

    /* Put our own polygon header */
    pvr_prim(&hdr, sizeof(hdr));

    /* Setup our Direct Render state: pick a store queue and setup QACR0/1 */
    plx_dr_init(&dr_state);

    /* Initialize xmtrx with the values from KGL */
    plx_mat_identity();
    plx_mat3d_apply(PLX_MAT_SCREENVIEW);
    plx_mat3d_apply(PLX_MAT_PROJECTION);
    plx_mat3d_apply(PLX_MAT_MODELVIEW);

    /* Iterate over stacks */
    for(i = 0; i < stacks; i++) {
        pitch = 2 * F_PI * ((float)i / (float)stacks);
        pitch2 = 2 * F_PI * ((float)(i + 1) / (float)stacks);

        /* Iterate over slices: each entire stack will be one
           long triangle strip. */
        for(j = 0; j <= slices / 2; j++) {
            yaw = 2 * F_PI * ((float)j / (float)slices);

            /* x, y+1 */
            x = radius * fcos(yaw) * fcos(pitch2);
            y = radius * fsin(pitch2);
            z = radius * fsin(yaw) * fcos(pitch2);
            mat_trans_single(x, y, z);          /* Use ftrv to transform */
            g = fcos(yaw * 2) / 2.0f + 0.5f;
            b = fsin(phase + pitch2) / 2.0f + 0.5f;
            plx_vert_fnd(&dr_state, PVR_CMD_VERTEX,
                         x, y, z, 0.5f,
                         0.0f, g, b);

            /* x, y */
            x = radius * fcos(yaw) * fcos(pitch);
            y = radius * fsin(pitch);
            z = radius * fsin(yaw) * fcos(pitch);
            mat_trans_single(x, y, z);
            g = fcos(yaw * 2) / 2.0f + 0.5f;
            b = fsin(phase + pitch) / 2.0f + 0.5f;

            if(j == (slices / 2)) {
                plx_vert_fnd(&dr_state, PVR_CMD_VERTEX_EOL,
                             x, y, z, 0.5f,
                             0.0f, g, b);
            }
            else {
                plx_vert_fnd(&dr_state, PVR_CMD_VERTEX,
                             x, y, z, 0.5f,
                             0.0f, g, b);
            }
        }
    }

    plx_dr_finish();
}

#define SPHERE_CNT 12
static float r = 0;
static void sphere_frame_opaque(void) {
    int i;

    vid_border_color(255, 0, 0);
    pvr_wait_ready();
    vid_border_color(0, 255, 0);

    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_OP_POLY);

    plx_mat3d_identity();
    plx_mat3d_translate(0.0f, 0.0f, -12.0f);
    plx_mat3d_rotate(r * 2, 0.75f, 1.0f, 0.5f);
    plx_mat3d_push();

    for(i = 0; i < SPHERE_CNT; i++) {
        plx_mat3d_translate(6.0f * fcos(i * 2 * F_PI / SPHERE_CNT), 0.0f, 6.0f * fsin(i * 2 * F_PI / SPHERE_CNT));
        plx_mat3d_rotate(r, 1.0f, 1.0f, 1.0f);
        sphere(1.2f, 20, 20);

        if(i < (SPHERE_CNT - 1))
            plx_mat3d_peek();
    }

    plx_mat3d_pop();

    plx_mat3d_identity();
    plx_mat3d_translate(0.0f, 0.0f, -12.0f);
    plx_mat3d_rotate(-r * 2, 0.75f, 1.0f, 0.5f);
    plx_mat3d_push();

    for(i = 0; i < SPHERE_CNT; i++) {
        plx_mat3d_translate(3.0f * fcos(i * 2 * F_PI / SPHERE_CNT), 0.0f, 3.0f * fsin(i * 2 * F_PI / SPHERE_CNT));
        plx_mat3d_rotate(r, 1.0f, 1.0f, 1.0f);
        sphere(0.8f, 20, 20);

        if(i < (SPHERE_CNT - 1))
            plx_mat3d_peek();
    }

    plx_mat3d_pop();

    vid_border_color(0, 0, 255);
    pvr_scene_finish();

    r++;
    phase += 2 * F_PI / 240.0f;
}

static void sphere_frame_trans(void) {
    int i;

    vid_border_color(255, 0, 0);
    pvr_wait_ready();
    vid_border_color(0, 255, 0);

    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_TR_POLY);

    plx_mat3d_identity();
    plx_mat3d_translate(0.0f, 0.0f, -12.0f);
    plx_mat3d_rotate(r * 2, 0.75f, 1.0f, 0.5f);
    plx_mat3d_push();

    for(i = 0; i < SPHERE_CNT; i++) {
        plx_mat3d_translate(4.0f * fcos(i * 2 * F_PI / SPHERE_CNT), 0.0f, 4.0f * fsin(i * 2 * F_PI / SPHERE_CNT));
        plx_mat3d_rotate(r, 1.0f, 1.0f, 1.0f);
        sphere(1.0f, 20, 20);

        if(i < (SPHERE_CNT - 1))
            plx_mat3d_peek();
    }

    plx_mat3d_pop();

    vid_border_color(0, 0, 255);
    pvr_scene_finish();

    r++;
    phase += 2 * F_PI / 240.0f;
}

void do_sphere_test(void) {
    maple_device_t *cont;
    cont_state_t *state;
    int     mode = 0;
    int     a_was_down = 0;

    pvr_set_bg_color(0.2f, 0.0f, 0.4f);

    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
    cxt.gen.culling = PVR_CULLING_NONE;
    pvr_poly_compile(&hdr, &cxt);

    for(;;) {
        if(!mode)
            sphere_frame_opaque();
        else
            sphere_frame_trans();

        cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

        if(!cont)
            continue;

        state = (cont_state_t *)maple_dev_status(cont);

        if(!state)
            continue;

        if(state->buttons & CONT_START) {
            return;
        }

        if(state->buttons & CONT_A) {
            if(a_was_down) continue;

            a_was_down = 1;
            mode ^= 1;

            switch(mode) {
                case 0: /* Opaque */
                    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
                    cxt.gen.culling = PVR_CULLING_NONE;
                    pvr_poly_compile(&hdr, &cxt);
                    break;
                case 1: /* Translucent */
                    pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
                    cxt.gen.culling = PVR_CULLING_NONE;
                    pvr_poly_compile(&hdr, &cxt);
                    break;
            }
        }
        else {
            a_was_down = 0;
        }
    }
}

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

int main(int argc, char **argv) {
    /* Init PVR API */
    if(pvr_init(&params) < 0)
        return -1;

    /* Init matrices */
    plx_mat3d_init();
    plx_mat3d_mode(PLX_MAT_PROJECTION);
    plx_mat3d_identity();
    plx_mat3d_perspective(45.0f, 640.0f / 480.0f, 0.1f, 100.0f);
    plx_mat3d_mode(PLX_MAT_MODELVIEW);

    /* Do the test */
    printf("Bubbles Parallax sample: press START to exit, A to toggle sphere type\n");
    do_sphere_test();

    return 0;
}

