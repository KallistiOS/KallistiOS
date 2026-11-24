/* KallistiOS ##version##

   examples/dreamcast/pvr/listcont_viewmodel/listcont_viewmodel.c

   A first-person "view model" with its own depth range, with list
   continuation (pvr_init_params_t::list_group_count).

   The camera stands right in front of a wall and holds a long object that
   would poke into it. Group 0 draws the room. At its end, in a presorted
   translucent list, a full-screen polygon with alpha 0, depth compare
   "always", depth write on and the farthest depth resets the depth buffer
   and leaves the colour alone. Group 1 then draws the held object in the
   opaque list: it is depth tested only against itself, and stays in front
   of the wall.

   With one group, the held object goes into the same opaque list as the
   room, and clips into the wall.

   The held object covers a few tiles with few polygons, so its group has
   8-word bins where the room has 16 (pvr_init_params_t::list_groups).

   Controls: A switches between one and two groups (the mode also changes by
   itself every 5 seconds until A is pressed), X toggles vertex DMA, Start
   exits. The mode and the PVR render time go to the serial console.
*/

#include <stdio.h>
#include <math.h>

#include <arch/timer.h>
#include <dc/pvr.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

static pvr_init_params_t params = {
    .opb_sizes = { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0 },
    .vertex_buf_size = 512 * 1024,
    .opb_overflow_count = 3,
};

/* Bins of each group with two groups: 16 words for the room, 8 for the
   held object */
static const pvr_list_group_params_t group_bins[2] = {
    { { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0 } },
    { { PVR_BINSIZE_8, PVR_BINSIZE_0, PVR_BINSIZE_8, PVR_BINSIZE_0, PVR_BINSIZE_0 } },
};

/* Vertex buffers for DMA mode (half of each per frame) */
#define VERTBUF_SIZE (128 * 1024)
static uint8_t op_buf[VERTBUF_SIZE] __attribute__((aligned(32)));
static uint8_t tr_buf[VERTBUF_SIZE] __attribute__((aligned(32)));

static int dma;

static pvr_poly_hdr_t hdr_op, hdr_tr, hdr_reset;

static void compile_headers(void) {
    pvr_poly_cxt_t cxt;

    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
    cxt.gen.culling = PVR_CULLING_NONE;
    pvr_poly_compile(&hdr_op, &cxt);

    pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
    cxt.gen.culling = PVR_CULLING_NONE;
    cxt.depth.write = PVR_DEPTHWRITE_DISABLE;
    pvr_poly_compile(&hdr_tr, &cxt);

    /* The depth reset: drawn whatever the depth, writes depth, and leaves
       the colour unchanged (alpha 0, and blending ZERO / ONE to be sure) */
    pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
    cxt.gen.culling = PVR_CULLING_NONE;
    cxt.depth.comparison = PVR_DEPTHCMP_ALWAYS;
    cxt.depth.write = PVR_DEPTHWRITE_ENABLE;
    cxt.blend.src = PVR_BLEND_ZERO;
    cxt.blend.dst = PVR_BLEND_ONE;
    pvr_poly_compile(&hdr_reset, &cxt);
}

/* (Re)initialize the PVR for the given number of groups and DMA setting */
static void setup(int groups) {
    static int cur_groups = -1, cur_dma = -1;

    if(groups == cur_groups && dma == cur_dma)
        return;

    if(cur_groups >= 0)
        pvr_shutdown();

    params.list_group_count = groups;
    params.list_groups = groups == 2 ? group_bins : NULL;
    params.dma_enabled = dma;
    pvr_init(&params);
    pvr_set_bg_color(0.1f, 0.1f, 0.1f);

    if(dma) {
        pvr_set_vertbuf(PVR_LIST_OP_POLY, op_buf, VERTBUF_SIZE);
        pvr_set_vertbuf(PVR_LIST_TR_POLY, tr_buf, VERTBUF_SIZE);
    }

    /* Group 0's translucent list must be presorted, so that the depth reset
       comes after the poster. pvr_init() resets the sort modes. */
    if(groups == 2)
        pvr_set_group_presort(0, true);

    compile_headers();
    cur_groups = groups;
    cur_dma = dma;
}

/* Controller: returns the buttons newly pressed since the last call */
static uint32_t buttons_pressed(void) {
    static uint32_t prev;
    maple_device_t *cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
    cont_state_t *st = cont ? (cont_state_t *)maple_dev_status(cont) : NULL;
    uint32_t now = st ? st->buttons : 0, pressed = now & ~prev;

    prev = now;
    return pressed;
}

/**** 3D: the camera is at the origin looking down +z, so world space is
      camera space (x right, y up) ***********************************/

typedef struct {
    float x, y, z;
} vec3_t;

static void vert(uint32_t flags, vec3_t c, uint32_t argb) {
    pvr_vertex_t v __attribute__((aligned(32)));

    v.flags = flags;
    v.x = 320.0f + 400.0f * c.x / c.z;
    v.y = 240.0f - 400.0f * c.y / c.z;
    v.z = 1.0f / c.z;
    v.u = v.v = 0.0f;
    v.argb = argb;
    v.oargb = 0;
    pvr_prim(&v, sizeof(v));
}

/* A quad with corners a, b, c, d in order around it */
static void quad(vec3_t a, vec3_t b, vec3_t c, vec3_t d, uint32_t argb) {
    vert(PVR_CMD_VERTEX, a, argb);
    vert(PVR_CMD_VERTEX, b, argb);
    vert(PVR_CMD_VERTEX, d, argb);
    vert(PVR_CMD_VERTEX_EOL, c, argb);
}

/* The wall right in front of the camera (at z = 1), and the floor */
static void room(void) {
    vec3_t a = { -3, -1.5f, 0.2f }, b = { -3, -1.5f, 1.0f },
           c = { 3, -1.5f, 1.0f }, d = { 3, -1.5f, 0.2f };
    int i, j;

    pvr_prim(&hdr_op, sizeof(hdr_op));

    for(i = -6; i < 6; i++) {
        for(j = -3; j < 4; j++) {
            vec3_t p = { i * 0.5f, j * 0.5f, 1.0f }, q = { p.x, p.y + 0.5f, 1.0f },
                   r = { p.x + 0.5f, p.y + 0.5f, 1.0f }, s = { p.x + 0.5f, p.y, 1.0f };

            quad(p, q, r, s, ((i + j) & 1) ? 0xff806040 : 0xffa08060);
        }
    }

    quad(a, b, c, d, 0xff404040);
}

/* A translucent poster on the wall */
static void poster(void) {
    vec3_t a = { -0.9f, -0.2f, 0.98f }, b = { -0.9f, 0.6f, 0.98f },
           c = { 0.1f, 0.6f, 0.98f }, d = { 0.1f, -0.2f, 0.98f };

    pvr_prim(&hdr_tr, sizeof(hdr_tr));
    quad(a, b, c, d, 0x8020c0ff);
}

/* The depth reset polygon, covering the whole screen at the farthest depth */
static void depth_reset(void) {
    pvr_vertex_t v __attribute__((aligned(32)));
    static const float corner[4][2] = { { 0, 480 }, { 0, 0 }, { 640, 480 }, { 640, 0 } };
    int i;

    pvr_prim(&hdr_reset, sizeof(hdr_reset));

    for(i = 0; i < 4; i++) {
        v.flags = i == 3 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
        v.x = corner[i][0];
        v.y = corner[i][1];
        v.z = 0.00001f;
        v.u = v.v = 0.0f;
        v.argb = 0x00000000;
        v.oargb = 0;
        pvr_prim(&v, sizeof(v));
    }
}

/* The held object: a long box from 0.5 to 2.2 in front of the camera,
   bobbing and turning a little */
static void held_object(float t) {
    static const int faces[6][4] = {
        { 0, 1, 2, 3 }, { 4, 5, 6, 7 }, { 0, 1, 5, 4 },
        { 1, 2, 6, 5 }, { 2, 3, 7, 6 }, { 3, 0, 4, 7 }
    };
    static const uint32_t colors[6] = {
        0xff303030, 0xffc0c0c0, 0xff909090, 0xff707070, 0xffa0a0a0, 0xff606060
    };
    static const float corner[4][2] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } };
    vec3_t c = { 0.42f, -0.35f + 0.03f * sinf(t * 2.0f), 1.35f };
    vec3_t h = { 0.08f, 0.08f, 0.85f };
    float angle = 0.15f + 0.05f * sinf(t), co = cosf(angle), si = sinf(angle);
    vec3_t v[8];
    int i;

    /* Corners 0-3 at the bottom, 4-7 at the top, in order around */
    for(i = 0; i < 8; i++) {
        float x = corner[i & 3][0] * h.x, z = corner[i & 3][1] * h.z;

        v[i] = (vec3_t){ c.x + x * co - z * si, c.y + ((i & 4) ? h.y : -h.y),
                         c.z + x * si + z * co };
    }

    pvr_prim(&hdr_op, sizeof(hdr_op));

    for(i = 0; i < 6; i++)
        quad(v[faces[i][0]], v[faces[i][1]], v[faces[i][2]], v[faces[i][3]],
             colors[i]);
}

static void do_frame(int two_groups, float t) {
    pvr_wait_ready();
    pvr_scene_begin();

    if(two_groups) {
        /* Group 0: the room, then reset the depth */
        pvr_list_begin(PVR_LIST_OP_POLY);
        room();
        pvr_list_begin(PVR_LIST_TR_POLY);
        poster();
        depth_reset();

        /* Group 1: the held object */
        pvr_list_continue();
        pvr_list_begin(PVR_LIST_OP_POLY);
        held_object(t);
    }
    else {
        pvr_list_begin(PVR_LIST_OP_POLY);
        room();
        held_object(t);
        pvr_list_begin(PVR_LIST_TR_POLY);
        poster();
    }

    pvr_scene_finish();
}

int main(int argc, char *argv[]) {
    uint32_t pressed;
    int two_groups = 1, autocycle = 1, frame = 0;
    uint64_t next_switch = timer_ms_gettime64() + 5000;
    pvr_stats_t stats;
    float t = 0.0f;

    (void)argc;
    (void)argv;

    printf("listcont_viewmodel: A = 1/2 groups, X = DMA, Start = exit\n");

    for(;;) {
        pressed = buttons_pressed();

        if(pressed & CONT_START)
            break;

        if(pressed & CONT_A) {
            two_groups = !two_groups;
            autocycle = 0;
        }

        if(pressed & CONT_X)
            dma = !dma;

        if(autocycle && timer_ms_gettime64() >= next_switch) {
            two_groups = !two_groups;
            next_switch = timer_ms_gettime64() + 5000;
        }

        setup(two_groups ? 2 : 1);

        if((++frame % 60) == 0) {
            pvr_get_stats(&stats);
            printf("listcont_viewmodel: %s%s, PVR render %.2f ms\n",
                   two_groups ? "2 groups: depth reset first" :
                                "1 group: object in the wall",
                   dma ? ", DMA" : "", (double)stats.rnd_last_time / 1e6);
        }

        do_frame(two_groups, t);
        t += 1.0f / 60.0f;
    }

    pvr_shutdown();

    return 0;
}
