/* GhettoPlay: an S3M browser and playback util
   (c)2000 Megan Potter
*/

#include "gp.h"


/* This module will manage the spiffy background effects */

static int ang = 0;
static pvr_ptr_t chktexture;
static float horizon = 340.0f;

/* Make a nice (now familiar =) XOR pattern texture */
void bkg_setup(void) {
    int x, y;
    uint16_t *texture;

    chktexture = pvr_mem_malloc(64 * 64 * 2);
    texture = (uint16_t *)chktexture;

    for(y = 0; y < 64; y++)
        for(x = 0; x < 64; x++) {
            int v = ((x * 4) ^(y * 4)) & 255;
            *texture++ = ((v >> 3) << 11)
                         | ((v >> 2) << 5)
                         | ((v >> 3) << 0);
        }
}

/* Draws the floor polygon */
static void draw_floor_poly(void) {
    pvr_vertex_t vert;
    float u2 = 0.374f, j = 0.0f;

    vert.flags = PVR_CMD_VERTEX;
    vert.x = 0.0f;
    vert.y = 480.0f;
    vert.z = 64.0f;
    vert.u = u2 - 0.5f;
    vert.v = 1.0f - 0.5f;
    rotate(ang, 0, 0, &vert.u, &vert.v, &j);
    vert.u += 0.5f;
    vert.v += 0.5f;
    vert.argb = PVR_PACK_COLOR(1.0f, 0.2f, 1.0f, 1.0f);
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = 0.0f;
    vert.y = horizon;
    vert.z = 16.0f;
    vert.u = 0.0f - 0.5f;
    vert.v = 0.0f - 0.5f;
    rotate(ang, 0, 0, &vert.u, &vert.v, &j);
    vert.u += 0.5f;
    vert.v += 0.5f;
    vert.argb = PVR_PACK_COLOR(1.0f, 0.5f, 0.3f, 1.0f);
    pvr_prim(&vert, sizeof(vert));

    vert.x = 640.0f;
    vert.y = 480.0f;
    vert.z = 64.0f;
    vert.u = (1.0f - u2) - 0.5f;
    vert.v = 1.0f - 0.5f;
    rotate(ang, 0, 0, &vert.u, &vert.v, &j);
    vert.u += 0.5f;
    vert.v += 0.5f;
    vert.argb = PVR_PACK_COLOR(1.0f, 0.2f, 1.0f, 1.0f);
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = 640.0f;
    vert.y = horizon;
    vert.z = 16.0f;
    vert.u = 1.0f - 0.5f;
    vert.v = 0.0f - 0.5f;
    rotate(ang, 0, 0, &vert.u, &vert.v, &j);
    vert.u += 0.5f;
    vert.v += 0.5f;
    vert.argb = PVR_PACK_COLOR(1.0f, 0.5f, 0.3f, 1.0f);
    pvr_prim(&vert, sizeof(vert));
}

/* Draws the "wall" polygon */
static void draw_wall_poly(void) {
    pvr_vertex_t    vertc;

    vertc.flags = PVR_CMD_VERTEX;
    vertc.x = 0.0f;
    vertc.y = horizon;
    vertc.z = 64.0f;
    vertc.argb = PVR_PACK_COLOR(1.0f, 0.5f, 0.3f, 1.0f);
    vertc.oargb = 0;
    pvr_prim(&vertc, sizeof(vertc));

    vertc.x = 0.0f;
    vertc.y = 0.0f;
    vertc.z = 64.0f;
    vertc.argb = PVR_PACK_COLOR(1.0f, 0.0f, 0.0f, 0.0f);
    pvr_prim(&vertc, sizeof(vertc));

    vertc.x = 640.0f;
    vertc.y = horizon;
    vertc.z = 64.0f;
    vertc.argb = PVR_PACK_COLOR(1.0f, 0.5f, 0.3f, 1.0f);
    pvr_prim(&vertc, sizeof(vertc));

    vertc.flags = PVR_CMD_VERTEX_EOL;
    vertc.x = 640.0f;
    vertc.y = 0.0f;
    vertc.z = 64.0f;
    vertc.argb = PVR_PACK_COLOR(1.0f, 0.0f, 0.0f, 0.0f);
    pvr_prim(&vertc, sizeof(vertc));
}

void bkg_render(void) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t poly;

    /* "floor" polygon */
    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565 | PVR_TXRFMT_NONTWIDDLED,
                     64, 64, chktexture, PVR_FILTER_BILINEAR);
    pvr_poly_compile(&poly, &cxt);
    pvr_prim(&poly, sizeof(poly));

    draw_floor_poly();

    /* "wall" polygon */
    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
    pvr_poly_compile(&poly, &cxt);
    pvr_prim(&poly, sizeof(poly));

    draw_wall_poly();

    ang = (ang - 1) & 255;
}

