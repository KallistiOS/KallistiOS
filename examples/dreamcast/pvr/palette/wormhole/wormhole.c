/* KallistiOS ##version##

   wormhole.c
   Copyright (C) 2024 Andress Barajas
*/

/*
   This demo demonstrates the use of 8-bit palette-based textures to generate
   and animate a wormhole effect on the Dreamcast. The pallete texture is created
   programmatically in RAM, simulating a wormhole with radial distortion based
   on distance and angle from the center, and then uploaded to VRAM for rendering.

   The wormhole texture consists of grayscale colors ranging from white to dark
   gray, while the background remains black. The colors in the wormhole are 
   continuously cycled in the palette, creating the appearance of a swirling 
   motion. The palette cycling is handled using a simple offset mechanism, 
   where the wormhole’s color indices shift over time based on the current frame.

   This effect demonstrates how to use 8-bit textures with dynamic palette 
   animation, giving the appearance of a smoothly animated wormhole. The player
   can press the START button on the controller to exit the demo.
*/

#include <stdlib.h>
#include <math.h>

#include <dc/pvr.h>
#include <dc/maple.h>
#include <dc/fmath.h>
#include <dc/maple/controller.h>

#define WORMHOLE_WIDTH 256
#define WORMHOLE_HEIGHT 256

/* Precomputed grayscale wormhole palette */
static const uint32_t wormhole_palette[32] = {
    0xFF000000,  /* Black */
    0xFF111111,  /* Very dark gray */
    0xFF222222,  /* Dark gray */
    0xFF333333,  /* Slightly lighter dark gray */
    0xFF444444,  /* Medium dark gray */
    0xFF555555,  /* Medium gray */
    0xFF666666,  /* Lighter medium gray */
    0xFF777777,  /* Light gray */
    0xFF888888,  /* Even lighter gray */
    0xFF999999,  /* Near light gray */
    0xFFAAAAAA,  /* Light gray */
    0xFFBBBBBB,  /* Brighter light gray */
    0xFFCCCCCC,  /* Very light gray */
    0xFFDDDDDD,  /* Nearly white gray */
    0xFFEEEEEE,  /* Very pale gray */
    0xFFFFFFFF,  /* White */
    /* Repeat for swirling effect */
    0xFF000000, 0xFF111111, 0xFF222222, 0xFF333333,
    0xFF444444, 0xFF555555, 0xFF666666, 0xFF777777,
    0xFF888888, 0xFF999999, 0xFFAAAAAA, 0xFFBBBBBB,
    0xFFCCCCCC, 0xFFDDDDDD, 0xFFEEEEEE, 0xFFFFFFFF
};

static pvr_poly_hdr_t hdr;

static void draw_screen() {
    pvr_vertex_t vert;

    pvr_prim(&hdr, sizeof(hdr));

    vert.argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert.oargb = 0;
    vert.flags = PVR_CMD_VERTEX;

    vert.x = 0.0f;
    vert.y = 0.0f;
    vert.z = 1.0f;
    vert.u = 0.0f;
    vert.v = 0.0f;
    pvr_prim(&vert, sizeof(vert));

    vert.x = 640.0f;
    vert.y = 0.0f;
    vert.z = 1.0f;
    vert.u = 1.0f;
    vert.v = 0.0f;
    pvr_prim(&vert, sizeof(vert));

    vert.x = 0.0f;
    vert.y = 480.0f;
    vert.z = 1.0f;
    vert.u = 0.0f;
    vert.v = 1.0f;
    pvr_prim(&vert, sizeof(vert));

    vert.x = 640.0f;
    vert.y = 480.0f;
    vert.z = 1.0f;
    vert.u = 1.0f;
    vert.v = 1.0f;
    vert.flags = PVR_CMD_VERTEX_EOL;
    pvr_prim(&vert, sizeof(vert));
}

static pvr_ptr_t generate_texture(uint32_t width, uint32_t height) {
    int x, y, index;
    float dx, dy, dist, angle;
    uint8_t *texbuf;
    pvr_ptr_t texptr;

    texbuf = calloc(width * height, sizeof(uint8_t));

    for(y = 0; y < height; y++)
        for(x = 0; x < width; x++) {
            /* Calculate distance from center for radial effect */
            dx = x - (float)width / 2;
            dy = y - (float)height / 2;
            dist = sqrtf(dx * dx + dy * dy);

            /* width/2 is the radius */
            if(dist < (float)width / 2) {
                /* Normalize angle to range [0, 31] */
                angle = (atan2f(dy, dx) + F_PI) * (31.0 / (2 * F_PI));

                /* Calculate palette index based on distance and angle, skipping index 0 */
                index = 1 + (int)(dist / 8 + angle) % 31;

                /* Use wormhole colors (1-31) */
                texbuf[y * width + x] = index;
            }
            else
                /* Outside wormhole is black (index 0) */
                texbuf[y * width + x] = 0; 
        }

    texptr = pvr_mem_malloc(width * height);
    pvr_txr_load_ex(texbuf, texptr, width, height, PVR_TXRLOAD_8BPP);

    free(texbuf);

    return texptr;
}

static void animate_wormhole(uint32_t frame) {
    /* Simulate movement by changing palette over time, keeping index 0 fixed */
    int offset = frame % 31;  
    for(int i = 1; i < 32; i++) /* Start from 1 to exclude index 0(BG color) */
        pvr_set_pal_entry(i, wormhole_palette[(i + offset) % 31 + 1]);
}

static int check_start(void) {
    MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)

    if(st->buttons & CONT_START)
        return 1;

    MAPLE_FOREACH_END()
    return 0;
}

int main(int argc, char** argv) {
    uint32_t frame = 0;
    pvr_ptr_t texptr;
    pvr_poly_cxt_t cxt;

    pvr_init_defaults();

    /* Set the palette format */
    pvr_set_pal_format(PVR_PAL_ARGB8888);

    /* Initialize the texture */
    texptr = generate_texture(WORMHOLE_WIDTH, WORMHOLE_HEIGHT);

    /* Set the background color */
    pvr_set_pal_entry(0, wormhole_palette[0]);

    /* Setup PVR context */
    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_PAL8BPP | 
                    PVR_TXRFMT_8BPP_PAL(0), WORMHOLE_WIDTH, WORMHOLE_HEIGHT, 
                    texptr, PVR_FILTER_BILINEAR);
    pvr_poly_compile(&hdr, &cxt);

    while(!check_start()) {
        frame++;

        animate_wormhole(frame);

        pvr_wait_ready();
        pvr_scene_begin();

        pvr_list_begin(PVR_LIST_OP_POLY);

        draw_screen();

        pvr_list_finish();
        pvr_scene_finish();
    }

    pvr_mem_free(texptr);

    return 0;
}
