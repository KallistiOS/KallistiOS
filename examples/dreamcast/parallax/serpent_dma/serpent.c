/*
   KallistiOS ##version##
   serpent.c

   Copyright (C) 2002,2004 Megan Potter
   Copyright (C) 2004 Jim Ursetto
*/

#include <kos.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <plx/matrix.h>
#include <plx/prim.h>
#include <plx/context.h>

/*

This is a copy of the Parallax "bubbles" demo that has been heavily modified
to work with the new mat_transform_sq and vertex DMA. It demonstrates basic
vertex DMA usage as well as showing off interleaved polygon type usage, etc.

*/

// These are in perfmeter.c
void pm_init();
void pm_draw();

/* Draws a sphere out of GL_QUADS according to the syntax of glutSolidSphere

   This now uses triangle strips and PVR Direct Rendering for the best
   possible performance.

   Based very loosely on a routine from James Susinno.
*/
static float phase = 0.0f;
static pvr_poly_cxt_t cxt[2];
static pvr_poly_hdr_t hdr[2];

typedef struct {
    float radius;
    int slices;
    int stacks;
    pvr_vertex_t *data;
} sphere_t;

static sphere_t big_sphere = { 1.2f, 20, 20, NULL };
static sphere_t small_sphere = { 0.8f, 20, 20, NULL };

static void sphere(sphere_t *s) { /* {{{ */
    int i, j;
    float   pitch, pitch2;
    float   r, g, b;
    float   yaw;
    pvr_vertex_t *v;

    s->data = (pvr_vertex_t *)aligned_alloc(32, s->stacks * (s->slices + 2) * sizeof(pvr_vertex_t));
    if(s->data == NULL) return;

    v = s->data;
    // s->data_trans = (pvr_vertex_t *)malloc(s->stacks * (s->slices+2) * sizeof(pvr_vertex_t));
    /* transformed data -- for testing mat_transform */
    printf("allocated %d bytes for %d stacks, %d + 2 slices, and %d-byte pvr_vertex_t\n",
           s->stacks * (s->slices + 2) * sizeof(pvr_vertex_t), s->stacks, s->slices, sizeof(pvr_vertex_t));

    /* Iterate over stacks */
    for(i = 0; i < s->stacks; i++) {
        pitch = 2 * F_PI * ((float)i / (float)s->stacks);
        pitch2 = 2 * F_PI * ((float)(i + 1) / (float)s->stacks);

        /* Iterate over slices: each entire stack will be one
           long triangle strip. */
        for(j = 0; j <= s->slices / 2; j++) {
            yaw = 2 * F_PI * ((float)j / (float)s->slices);

            /* x, y+1 */
            v->x = s->radius * fcos(yaw) * fcos(pitch2);
            v->y = s->radius * fsin(pitch2);
            v->z = s->radius * fsin(yaw) * fcos(pitch2);


            r = 0.75f;
            g = fcos(yaw * 2) / 3.0f + 0.35f;
            b = 0.0f;

            v->argb = plx_pack_color(0.5f, r, g, b);
            v->flags = PVR_CMD_VERTEX;
            v++;


            /* x, y */
            v->x = s->radius * fcos(yaw) * fcos(pitch);
            v->y = s->radius * fsin(pitch);
            v->z = s->radius * fsin(yaw) * fcos(pitch);

            //r = 0.5f;
            r = 0.75f;
            g = fsin(yaw * 2) / 3.0f + 0.35f;
            b = 0.0f;
            // b = fsin( phase + pitch ) / 2.0f + 0.5f;

            v->argb = plx_pack_color(0.5f, r, g, b);

            if(j == s->slices / 2)
                v->flags = PVR_CMD_VERTEX_EOL;
            else
                v->flags = PVR_CMD_VERTEX;

            v++;
        }
    }
}

static void draw_sphere(sphere_t *s, int list) {
    void * sqd;
    pvr_vertex_t *v, *vd;

    v = s->data;
    assert(v != NULL);

    /* Initialize xmtrx with the values from KGL */
    plx_mat_identity();
    plx_mat3d_apply(PLX_MAT_SCREENVIEW);
    plx_mat3d_apply(PLX_MAT_PROJECTION);
    plx_mat3d_apply(PLX_MAT_MODELVIEW);

    irq_disable();

    /* Put our own polygon header */
    if(list == PVR_LIST_OP_POLY)
        pvr_list_prim(list, &hdr[0], sizeof(hdr[0]));
    else
        pvr_list_prim(list, &hdr[1], sizeof(hdr[1]));

    /* Transform and write vertices to the TA via the store queues */
    vd = (pvr_vertex_t *)pvr_vertbuf_tail(list);
    sq_lock(vd);
    sqd = (void *) SQ_MASK_DEST_ADDR(vd);
    /* {
        int o = irq_disable();
        printf("transforming to %p, len %d\n",
            vd, 32 * s->stacks * (s->slices + 2));
        irq_restore(o);
    } */

    mat_transform_sq(v, sqd, s->stacks * (s->slices + 2));
    sq_unlock();

    pvr_vertbuf_written(list, 32 * s->stacks * (s->slices + 2));

    irq_enable();
}

#define SPHERE_CNT 32
static float r = 0;
static void sphere_frame(void) {
    int i;
    //uint64_t start;

    if(!big_sphere.data)
        sphere(&big_sphere);

    if(!small_sphere.data)
        sphere(&small_sphere);

    pvr_wait_ready();
    pvr_scene_begin();

    //start = timer_ms_gettime64();

    plx_mat3d_identity();
    plx_mat3d_translate(0.0f, 0.0f, -12.0f);
    plx_mat3d_rotate(r * 2, 0.75f, 1.0f, 0.5f);
    plx_mat3d_push();

    for(i = 0; i < SPHERE_CNT; i++) {
        plx_mat3d_translate(6.0f * fcos(i * 2 * F_PI / SPHERE_CNT), 0.0f, 6.0f * fsin(i * 2 * F_PI / SPHERE_CNT));
        plx_mat3d_rotate(r, 1.0f, 1.0f, 1.0f);

        if(!(i % 2))
            draw_sphere(&big_sphere, PVR_LIST_OP_POLY);
        else
            draw_sphere(&big_sphere, PVR_LIST_TR_POLY);

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

        if(!(i % 2))
            draw_sphere(&small_sphere, PVR_LIST_OP_POLY);
        else
            draw_sphere(&small_sphere, PVR_LIST_TR_POLY);

        if(i < (SPHERE_CNT - 1))
            plx_mat3d_peek();
    }

    plx_mat3d_pop();

    // Fake spending more time on vertices
    //timer_spin_sleep(15);

    //printf("%d\n", (uint32_t)(timer_ms_gettime64() - start));

    r++;
    phase += 2 * F_PI / 240.0f;

    pm_draw();

    pvr_scene_finish();
}

void do_sphere_test(void) {
    pvr_set_bg_color(0.2f, 0.0f, 0.4f);

    pvr_poly_cxt_col(&cxt[0], PVR_LIST_OP_POLY);
    pvr_poly_cxt_col(&cxt[1], PVR_LIST_TR_POLY);
    cxt[0].gen.culling = PVR_CULLING_NONE;
    cxt[1].gen.culling = PVR_CULLING_NONE;
    pvr_poly_compile(&hdr[0], &cxt[0]);
    pvr_poly_compile(&hdr[1], &cxt[1]);

    for(;;) {
        sphere_frame();

        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)

        if(st->buttons & CONT_START)
            return;

        MAPLE_FOREACH_END()
    }
}

pvr_init_params_t params = {
    /* Enable opaque and translucent polygons with binsize 32 */
    { PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_0 },

    /* Vertex buffer size */
    2560 * 1024,

    /* Vertex DMA enabled */
    1,

    /* No FSAA */
    0,

    /* Translucent Autosort enabled */
    0,

    /* Extra OPBs */
    3,

    /* Vertex buffer double-buffering enabled */
    0
};

// DMA buffers. This should ideally be in separate memory banks to take
// advantage of greater speed, but this will work for now.
uint8_t dmabuffers[2][4 * 1024 * 1024] __attribute__((aligned(32)));

int main(int argc, char **argv) {
    /* Init PVR API */
    pvr_stats_t stats;

    if(pvr_init(&params) < 0)
        return -1;

    pvr_set_vertbuf(PVR_LIST_OP_POLY, dmabuffers[0], 4 * 1024 * 1024);
    pvr_set_vertbuf(PVR_LIST_TR_POLY, dmabuffers[1], 4 * 1024 * 1024);

    /* Init matrices */
    plx_mat3d_init();
    plx_mat3d_mode(PLX_MAT_PROJECTION);
    plx_mat3d_identity();
    plx_mat3d_perspective(45.0f, 640.0f / 480.0f, 0.1f, 100.0f);
    plx_mat3d_mode(PLX_MAT_MODELVIEW);

    plx_cxt_init();
    pm_init();

    /* Do the test */
    printf("Serpent sample: press START to exit\n");
    do_sphere_test();

    pvr_get_stats(&stats);
    dbglog(DBG_DEBUG, "3D Stats: %u vblanks, frame rate ~%f fps, max vertex used %u bytes\n",
           stats.vbl_count, stats.frame_rate, stats.vtx_buffer_used_max);

    free(big_sphere.data);
    free(small_sphere.data);

    return 0;
}

