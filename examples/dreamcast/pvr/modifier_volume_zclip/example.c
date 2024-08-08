/* Modifier volume and Zclipping example for KOS 2.1.x
 * by Twada
 */
#include <kos.h>
#include <png/png.h>
#include <zlib/zlib.h>
#include "pvr_zclip.h"

/* textures */
pvr_ptr_t box_tex;

void mul_screen(float width, float height)
{
    __attribute__((aligned(32))) static matrix_t d = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}};
    d[0][0] = width * 0.5f;
    d[1][1] = -height * 0.5f;
    d[3][0] = width * 0.5f;
    d[3][1] = height * 0.5f;
    mat_apply(&d);
}

void mul_projection(float fov, float aspect, float znear)
{
    __attribute__((aligned(32))) static matrix_t d = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}};
    float s = 1.0f / ftan(fov * 0.5f);
    d[0][0] = s / aspect;
    d[1][1] = s;
    d[2][2] = 0.0f;
    d[2][3] = -1.0f;
    d[3][2] = znear;
    d[3][3] = 0.0f;
    mat_apply(&d);
}

void draw_modifier(float *pvm)
{
    __attribute__((aligned(32))) static pvr_modifier_vol_t vol[12] = {
        {PVR_CMD_VERTEX_EOL, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0},
        {PVR_CMD_VERTEX_EOL, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0},
        {PVR_CMD_VERTEX_EOL, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0},
        {PVR_CMD_VERTEX_EOL, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0},
        {PVR_CMD_VERTEX_EOL, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0},
        {PVR_CMD_VERTEX_EOL, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0},
        {PVR_CMD_VERTEX_EOL, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0},
        {PVR_CMD_VERTEX_EOL, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0},
        {PVR_CMD_VERTEX_EOL, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0},
        {PVR_CMD_VERTEX_EOL, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0},
        {PVR_CMD_VERTEX_EOL, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0},
        {PVR_CMD_VERTEX_EOL, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0},
    };
    float vert[8][4] = {
        {-1.0f, -1.0f, 1.0f, 1.0f},
        {-1.0f, -1.0f, -1.0f, 1.0f},
        {1.0f, -1.0f, 1.0f, 1.0f},
        {1.0f, -1.0f, -1.0f, 1.0f},
        {-1.0f, 1.0f, 1.0f, 1.0f},
        {-1.0f, 1.0f, -1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, -1.0f, 1.0f},
    };
    float transform[8][4];
    int index[12][3] = {
        {0, 1, 2}, /* Bottom */
        {2, 1, 3},
        {4, 5, 6}, /* Top */
        {6, 5, 7},
        {0, 4, 2}, /* Front */
        {2, 4, 6},
        {2, 6, 3}, /* Right */
        {3, 6, 7},
        {1, 5, 3}, /* Back */
        {3, 5, 7},
        {1, 5, 0}, /* Left */
        {0, 5, 4}};
    pvr_mod_hdr_t hdr;
    static float ry = 0.0f;
    /* Vertex transform */
    ry += 0.01;
    mat_identity();
    mat_apply((matrix_t *)pvm);
    mat_translate(-1.0f, 0.25f, 1.0f);
    mat_rotate(0.0f, ry, 0.0f);
    mat_scale(2.0f, 2.0f, 2.0f);
    for (int i = 0; i < 8; i++)
    {
        float tmp[4] = {vert[i][0], vert[i][1], vert[i][2], vert[i][3]};
        mat_trans_nodiv(tmp[0], tmp[1], tmp[2], tmp[3]);
        transform[i][0] = tmp[0] / tmp[3];
        transform[i][1] = tmp[1] / tmp[3];
        transform[i][2] = tmp[2] / tmp[3];
        transform[i][3] = 1.0f;
    }
    /* Face set */
    for (int i = 0; i < 12; i++)
    {
        vol[i].ax = transform[index[i][0]][0];
        vol[i].ay = transform[index[i][0]][1];
        vol[i].az = transform[index[i][0]][2];
        vol[i].bx = transform[index[i][1]][0];
        vol[i].by = transform[index[i][1]][1];
        vol[i].bz = transform[index[i][1]][2];
        vol[i].cx = transform[index[i][2]][0];
        vol[i].cy = transform[index[i][2]][1];
        vol[i].cz = transform[index[i][2]][2];
    }
    pvr_mod_compile(&hdr, PVR_LIST_OP_MOD, PVR_MODIFIER_INCLUDE_LAST_POLY, PVR_CULLING_SMALL);
    hdr.cmd |= (1 << 6); /* Last poly */
    pvr_modifier_commit_zclip(&hdr, vol, 12);
}

void draw_box(float *pvm)
{
    __attribute__((aligned(32))) static pvr_vertex_t poly[18] = {
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0xffffffff, 0x00000000},
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xffffffff, 0x00000000},
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0xffffffff, 0x00000000},
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0xffffffff, 0x00000000},
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0xffffffff, 0x00000000},
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xffffffff, 0x00000000},
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0xffffffff, 0x00000000},
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0xffffffff, 0x00000000},
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0xffffffff, 0x00000000},
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xffffffff, 0x00000000},
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0xffffffff, 0x00000000},
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0xffffffff, 0x00000000},
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0xffffffff, 0x00000000},
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xffffffff, 0x00000000},
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0xffffffff, 0x00000000},
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0xffffffff, 0x00000000},
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0xffffffff, 0x00000000},
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xffffffff, 0x00000000},
    };
    float vert[8][4] = {
        {-1.0f, -1.0f, 1.0f, 1.0f},
        {-1.0f, -1.0f, -1.0f, 1.0f},
        {1.0f, -1.0f, 1.0f, 1.0f},
        {1.0f, -1.0f, -1.0f, 1.0f},
        {-1.0f, 1.0f, 1.0f, 1.0f},
        {-1.0f, 1.0f, -1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, -1.0f, 1.0f},
    };
    float transform[8][4];
    int index[18] = {
        1, 0, 3, -2,
        4, 5, 6, -7,
        0, 4, 2, 6, 3, 7, 1, 5, 0, -4};
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    /* Vertex transform */
    mat_identity();
    mat_apply((matrix_t *)pvm);
    mat_translate(1.0f, 2.0f, -1.0f);
    mat_scale(2.0f, 2.0f, 2.0f);
    for (int i = 0; i < 8; i++)
    {
        float tmp[4] = {vert[i][0], vert[i][1], vert[i][2], vert[i][3]};
        mat_trans_nodiv(tmp[0], tmp[1], tmp[2], tmp[3]);
        transform[i][0] = tmp[0] / tmp[3];
        transform[i][1] = tmp[1] / tmp[3];
        transform[i][2] = tmp[2] / tmp[3];
        transform[i][3] = 1.0f;
    }
    /* Face set */
    for (int i = 0; i < 18; i++)
    {
        int idx = index[i];
        if (idx < 0)
        {
            poly[i].flags = PVR_CMD_VERTEX_EOL;
            idx = -idx;
        }
        else
        {
            poly[i].flags = PVR_CMD_VERTEX;
        }
        poly[i].x = transform[idx][0];
        poly[i].y = transform[idx][1];
        poly[i].z = transform[idx][2];
    }
    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565, 256, 256, box_tex, PVR_FILTER_BILINEAR);
    pvr_poly_compile(&hdr, &cxt);
    hdr.cmd |= (1 << 7); /* PVR_MODIFIER_CHEAP_SHADOW */
    pvr_prim(&hdr, sizeof(hdr));
    pvr_vertex_commit_zclip(poly, 18);
}

void draw_plane(float *pvm)
{
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    __attribute__((aligned(32))) static pvr_vertex_t poly[4] = {
        {PVR_CMD_VERTEX, 0.0f, 100.0f, 0.5f, 0.0f, 1.0f, 0xffff0000, 0x00000000},
        {PVR_CMD_VERTEX, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0xff00ff00, 0x00000000},
        {PVR_CMD_VERTEX, 100.0f, 100.0f, 0.5f, 1.0f, 1.0f, 0xff0000ff, 0x00000000},
        {PVR_CMD_VERTEX_EOL, 100.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0xffffffff, 0x00000000},
    };
    float vert[4][4] = {
        {-5.0f, 0.0f, 5.0f, 1.0f},
        {-5.0f, 0.0f, -5.0f, 1.0f},
        {5.0f, 0.0f, 5.0f, 1.0f},
        {5.0f, 0.0f, -5.0f, 1.0f}};
    int i;
    mat_identity();
    mat_apply((matrix_t *)pvm);
    for (i = 0; i < 4; i++)
    {
        float tmp[4] = {vert[i][0], vert[i][1], vert[i][2], vert[i][3]};
        mat_trans_nodiv(tmp[0], tmp[1], tmp[2], tmp[3]);
        poly[i].x = tmp[0] / tmp[3];
        poly[i].y = tmp[1] / tmp[3];
        poly[i].z = tmp[2] / tmp[3];
    }
    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
    pvr_poly_compile(&hdr, &cxt);
    hdr.cmd |= (1 << 7); /* PVR_MODIFIER_CHEAP_SHADOW */
    pvr_prim(&hdr, sizeof(hdr));
    pvr_vertex_commit_zclip(poly, 4);
}

int main(void)
{
    pvr_init_params_t params = {
        /* Enable opaque and translucent polygons with size 16 */
        .opb_sizes = {PVR_BINSIZE_16, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0, PVR_BINSIZE_0},
        .vertex_buf_size = 512 * 1024, /* Vertex buffer size 512K */
        .dma_enabled = 0,              /* No DMA */
        .fsaa_enabled = 0,             /* No FSAA */
        .autosort_disabled = 0,        /* Translucent Autosort enabled. */
        .opb_overflow_count = 3        /* Extra OPBs */
    };
    static float cam_pvm[16];
    point_t cam_pos = {0.0f, 0.0f, 1.0f, 1.0f};
    point_t cam_tar = {0.0f, 0.0f, 0.0f, 1.0f};
    point_t cam_up = {0.0f, 1.0f, 0.0f, 1.0f};
    int done = 0;
    float dy = 0.0f;
    float dis = 5.0f;
    float high = 2.0f;

    /* init kos  */
    pvr_init(&params);
    pvr_set_bg_color(0, 0.5f, 1.0f);
    /* (cheap_shadow << 8) | (int)(intensity * 255) */
    PVR_SET(PVR_CHEAP_SHADOW, 0x00000180);

    /* init plane */
    box_tex = pvr_mem_malloc(256 * 256 * 2);
    png_to_texture("/rd/blocks.png", box_tex, PNG_NO_ALPHA);

    /* keep drawing frames until start is pressed */
    while (!done)
    {
        maple_device_t *dev = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
        if (dev)
        {

            cont_state_t *st = (cont_state_t *)maple_dev_status(dev);
            dy += (float)st->joyx * (0.1f / 127.0f);
            high -= (float)st->joyy * (0.25f / 127.0f);
            if (st->rtrig)
                dis -= (float)st->rtrig * 0.001f;
            if (st->ltrig)
                dis += (float)st->ltrig * 0.001f;
            cam_pos.x = fsin(dy) * dis;
            cam_pos.y = high;
            cam_pos.z = fcos(dy) * dis;

            if (st->buttons & CONT_START)
                done = 1;
        }
        /* set camera */
        mat_identity();
        mul_screen(640.0f, 480.0f);
        mul_projection((3.14159265f / 3.0f), (640.0f / 480.0f), 0.125f);
        mat_lookat(&cam_pos, &cam_tar, &cam_up);
        mat_store((matrix_t *)cam_pvm);

        pvr_wait_ready();
        pvr_scene_begin();

        pvr_list_begin(PVR_LIST_OP_POLY);
        draw_plane(cam_pvm);
        draw_box(cam_pvm);
        pvr_list_finish();

        pvr_list_begin(PVR_LIST_OP_MOD);
        draw_modifier(cam_pvm);
        pvr_list_finish();

        pvr_scene_finish();
    }
    pvr_mem_free(box_tex);

    return 0;
}
