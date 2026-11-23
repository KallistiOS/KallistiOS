/* KallistiOS ##version##

   examples/dreamcast/pvr/listcont_mirror/listcont_mirror.c

   A rear-view mirror drawn in a second list group
   (pvr_init_params_t::list_group_count).

   Group 0 draws the view ahead: road, posts, and translucent banners over
   the road. Group 1 draws the mirror. It starts with an opaque rectangle
   over the mirror area, with depth compare "always" and the farthest
   depth: that gives the mirror its backdrop and clears the depth there.
   Then it draws the scene seen from a camera looking back, with its own
   opaque and translucent lists, clipped to the mirror (PVR_USERCLIP_INSIDE;
   the mirror is aligned to 32-pixel tiles).

   With one group, the banners of the view ahead are drawn over the mirror,
   and the translucent polygons of both views are sorted together.

   The mirror covers a few tiles with few polygons, so its group has 8-word
   bins where the view ahead has 16 (pvr_init_params_t::list_groups).

   Controls: A switches between one and two groups (the mode also changes by
   itself every 5 seconds until A is pressed), X toggles vertex DMA, Start
   exits. The mode and the PVR render time go to the serial console.
*/

#include <stdio.h>
#include <string.h>
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

/* Bins of each group with two groups: 16 words for the view ahead, 8 for
   the mirror */
static const pvr_list_group_params_t group_bins[2] = {
    { { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0 } },
    { { PVR_BINSIZE_8, PVR_BINSIZE_0, PVR_BINSIZE_8, PVR_BINSIZE_0, PVR_BINSIZE_0 } },
};

/* Vertex buffers for DMA mode (half of each per frame) */
#define VERTBUF_SIZE (256 * 1024)
static uint8_t op_buf[VERTBUF_SIZE] __attribute__((aligned(32)));
static uint8_t tr_buf[VERTBUF_SIZE] __attribute__((aligned(32)));

static int dma;

static pvr_poly_hdr_t hdr_op, hdr_tr, hdr_backdrop, hdr_op_clip, hdr_tr_clip;

static void compile_headers(void) {
    pvr_poly_cxt_t cxt;

    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
    cxt.gen.culling = PVR_CULLING_NONE;
    pvr_poly_compile(&hdr_op, &cxt);

    cxt.gen.clip_mode = PVR_USERCLIP_INSIDE;
    pvr_poly_compile(&hdr_op_clip, &cxt);

    /* Mirror backdrop: drawn whatever the depth, writes the farthest depth */
    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
    cxt.gen.culling = PVR_CULLING_NONE;
    cxt.depth.comparison = PVR_DEPTHCMP_ALWAYS;
    pvr_poly_compile(&hdr_backdrop, &cxt);

    pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
    cxt.gen.culling = PVR_CULLING_NONE;
    cxt.depth.write = PVR_DEPTHWRITE_DISABLE;
    pvr_poly_compile(&hdr_tr, &cxt);

    cxt.gen.clip_mode = PVR_USERCLIP_INSIDE;
    pvr_poly_compile(&hdr_tr_clip, &cxt);
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
    pvr_set_bg_color(0.5f, 0.7f, 0.95f);

    if(dma) {
        pvr_set_vertbuf(PVR_LIST_OP_POLY, op_buf, VERTBUF_SIZE);
        pvr_set_vertbuf(PVR_LIST_TR_POLY, tr_buf, VERTBUF_SIZE);
    }

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

/**** 3D ***********************************************************/

typedef struct {
    float x, y, z;
} vec3_t;

/* Camera position and heading; camera space: x right, y up, z forward */
static vec3_t cam;
static float cam_yaw;
static float proj_cx, proj_cy, proj_fx, proj_fy;

static vec3_t to_camera(vec3_t p) {
    float x = p.x - cam.x, y = p.y - cam.y, z = p.z - cam.z;
    float cy = cosf(cam_yaw), sy = sinf(cam_yaw);
    vec3_t r = { x * cy - z * sy, y, x * sy + z * cy };

    return r;
}

static void vert(uint32_t flags, vec3_t p, uint32_t argb) {
    pvr_vertex_t v __attribute__((aligned(32)));
    vec3_t c = to_camera(p);

    v.flags = flags;
    v.x = proj_cx + proj_fx * c.x / c.z;
    v.y = proj_cy - proj_fy * c.y / c.z;
    v.z = 1.0f / c.z;
    v.u = v.v = 0.0f;
    v.argb = argb;
    v.oargb = 0;
    pvr_prim(&v, sizeof(v));
}

/* A quad with corners a, b, c, d in order around it, in world space */
static void quad(vec3_t a, vec3_t b, vec3_t c, vec3_t d, uint32_t argb) {
    vert(PVR_CMD_VERTEX, a, argb);
    vert(PVR_CMD_VERTEX, b, argb);
    vert(PVR_CMD_VERTEX, d, argb);
    vert(PVR_CMD_VERTEX_EOL, c, argb);
}

/* A screen-space rectangle */
static void rect(float x0, float y0, float x1, float y1, float z, uint32_t argb) {
    pvr_vertex_t v __attribute__((aligned(32)));

    v.u = v.v = 0.0f;
    v.argb = argb;
    v.oargb = 0;
    v.z = z;
    v.flags = PVR_CMD_VERTEX;
    v.x = x0; v.y = y1;
    pvr_prim(&v, sizeof(v));
    v.y = y0;
    pvr_prim(&v, sizeof(v));
    v.x = x1; v.y = y1;
    pvr_prim(&v, sizeof(v));
    v.flags = PVR_CMD_VERTEX_EOL;
    v.y = y0;
    pvr_prim(&v, sizeof(v));
}

/**** Scene ********************************************************/

/* The mirror, in tiles (inclusive) and in pixels */
#define MIRROR_TX0  7
#define MIRROR_TY0  3
#define MIRROR_TX1  12
#define MIRROR_TY1  5
#define MIRROR_X0   (MIRROR_TX0 * 32)
#define MIRROR_Y0   (MIRROR_TY0 * 32)
#define MIRROR_X1   ((MIRROR_TX1 + 1) * 32)
#define MIRROR_Y1   ((MIRROR_TY1 + 1) * 32)

static float drive_z;

/* Is a world-space point in front of the camera? */
static int visible(vec3_t p) {
    return to_camera(p).z > 0.5f;
}

/* Road and posts from z0 to z1 as seen from the current camera */
static void scene_op(float z0, float z1) {
    vec3_t a = { -4, 0, z0 }, b = { -4, 0, z1 }, c = { 4, 0, z1 }, d = { 4, 0, z0 };
    vec3_t e = { -0.1f, 0.01f, z0 }, f = { -0.1f, 0.01f, z1 },
           g = { 0.1f, 0.01f, z1 }, h = { 0.1f, 0.01f, z0 };
    float z, x;
    int side;

    quad(a, b, c, d, 0xff505058);
    quad(e, f, g, h, 0xfff0f0f0);

    /* A post on each side every 4 units: front face white, side red */
    for(z = ceilf(z0 / 4.0f) * 4.0f; z < z1; z += 4.0f) {
        for(side = -1; side <= 1; side += 2) {
            vec3_t p0, p1, p2, p3;

            x = side * 4.5f;
            p0 = (vec3_t){ x - 0.15f, 0.0f, z };
            p1 = (vec3_t){ x - 0.15f, 1.6f, z };
            p2 = (vec3_t){ x + 0.15f, 1.6f, z };
            p3 = (vec3_t){ x + 0.15f, 0.0f, z };

            if(!visible(p0))
                continue;

            quad(p0, p1, p2, p3, 0xffd0d0d0);
            p0.x = p1.x = p2.x = p3.x = x - side * 0.15f;
            p2.z = p3.z = z + 0.3f;
            quad(p0, p1, p2, p3, 0xffe02020);
        }
    }
}

/* Translucent banners across the road every 20 units */
static void scene_tr(float z0, float z1) {
    float z;

    for(z = ceilf(z0 / 20.0f) * 20.0f + 10.0f; z < z1; z += 20.0f) {
        vec3_t a = { -4.5f, 2.0f, z }, b = { -4.5f, 4.0f, z },
               c = { 4.5f, 4.0f, z }, d = { 4.5f, 2.0f, z };

        if(visible(a))
            quad(a, b, c, d, 0xa0ffa000);
    }
}

/* Cameras: ahead (full screen) and back (mirror, flipped left to right) */
static void camera(int back) {
    cam.x = 0.8f; cam.y = 1.2f; cam.z = drive_z;
    cam_yaw = back ? (float)M_PI : 0.0f;
    proj_cx = back ? (MIRROR_X0 + MIRROR_X1) / 2.0f : 320.0f;
    proj_cy = back ? (MIRROR_Y0 + MIRROR_Y1) / 2.0f : 240.0f;
    proj_fx = back ? -150.0f : 400.0f;
    proj_fy = back ? 150.0f : 400.0f;
}

/* Send the mirror's user clip area (in tiles) to the open list */
static void user_clip(void) {
    struct {
        uint32_t cmd, pad[3], x0, y0, x1, y1;
    } __attribute__((aligned(32))) uc;

    memset(&uc, 0, sizeof(uc));
    uc.cmd = PVR_CMD_USERCLIP;
    uc.x0 = MIRROR_TX0;
    uc.y0 = MIRROR_TY0;
    uc.x1 = MIRROR_TX1;
    uc.y1 = MIRROR_TY1;
    pvr_prim(&uc, sizeof(uc));
}

static void ahead_op(void) {
    camera(0);
    pvr_prim(&hdr_op, sizeof(hdr_op));
    scene_op(drive_z + 0.3f, drive_z + 80.0f);
}

static void ahead_tr(void) {
    camera(0);
    pvr_prim(&hdr_tr, sizeof(hdr_tr));
    scene_tr(drive_z, drive_z + 80.0f);
}

static void mirror_op(void) {
    /* Backdrop and depth reset */
    pvr_prim(&hdr_backdrop, sizeof(hdr_backdrop));
    rect(MIRROR_X0, MIRROR_Y0, MIRROR_X1, MIRROR_Y1, 0.00001f, 0xff6080c0);

    /* The view back, clipped to the mirror */
    camera(1);
    user_clip();
    pvr_prim(&hdr_op_clip, sizeof(hdr_op_clip));
    scene_op(drive_z - 60.0f, drive_z - 0.3f);

    /* Frame, at the nearest depth */
    pvr_prim(&hdr_op, sizeof(hdr_op));
    rect(MIRROR_X0 - 6, MIRROR_Y0 - 6, MIRROR_X1 + 6, MIRROR_Y0, 100.0f, 0xff101010);
    rect(MIRROR_X0 - 6, MIRROR_Y1, MIRROR_X1 + 6, MIRROR_Y1 + 6, 100.0f, 0xff101010);
    rect(MIRROR_X0 - 6, MIRROR_Y0, MIRROR_X0, MIRROR_Y1, 100.0f, 0xff101010);
    rect(MIRROR_X1, MIRROR_Y0, MIRROR_X1 + 6, MIRROR_Y1, 100.0f, 0xff101010);
}

static void mirror_tr(void) {
    camera(1);
    user_clip();
    pvr_prim(&hdr_tr_clip, sizeof(hdr_tr_clip));
    scene_tr(drive_z - 60.0f, drive_z);
}

static void do_frame(int two_groups) {
    pvr_wait_ready();
    pvr_scene_begin();

    if(two_groups) {
        /* Group 0: the view ahead */
        pvr_list_begin(PVR_LIST_OP_POLY);
        ahead_op();
        pvr_list_begin(PVR_LIST_TR_POLY);
        ahead_tr();

        /* Group 1: the mirror */
        pvr_list_continue();
        pvr_list_begin(PVR_LIST_OP_POLY);
        mirror_op();
        pvr_list_begin(PVR_LIST_TR_POLY);
        mirror_tr();
    }
    else {
        pvr_list_begin(PVR_LIST_OP_POLY);
        ahead_op();
        mirror_op();
        pvr_list_begin(PVR_LIST_TR_POLY);
        ahead_tr();
        mirror_tr();
    }

    pvr_scene_finish();
}

int main(int argc, char *argv[]) {
    uint32_t pressed;
    int two_groups = 1, autocycle = 1, frame = 0;
    uint64_t next_switch = timer_ms_gettime64() + 5000;
    pvr_stats_t stats;

    (void)argc;
    (void)argv;

    printf("listcont_mirror: A = 1/2 groups, X = DMA, Start = exit\n");

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
            printf("listcont_mirror: %s%s, PVR render %.2f ms\n",
                   two_groups ? "2 groups: mirror in group 1" :
                                "1 group: banners over mirror",
                   dma ? ", DMA" : "", (double)stats.rnd_last_time / 1e6);
        }

        do_frame(two_groups);
        drive_z += 0.25f;
    }

    pvr_shutdown();

    return 0;
}
