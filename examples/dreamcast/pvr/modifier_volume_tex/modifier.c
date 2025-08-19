/* KallistiOS ##version##

   examples/dreamcast/pvr/modifier_volume_tex/modifier.c
   Copyright (C) 2009 Lawrence Sebald

   This example shows off how to do a basic modifier volume (with textures).
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <dc/pvr.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

#include <kos/init.h>

#include <pcx/pcx.h>
#include <kmg/kmg.h>

#define NUM_POLYS 10

static pvr_vertex_tpcm_t verts[NUM_POLYS * 4];

static pvr_poly_mod_hdr_t phdr;
static pvr_mod_hdr_t mhdr, mhdr2;
static float mx = 320.0f, my = 240.0f;
static pvr_list_t list = PVR_LIST_OP_POLY;
static pvr_ptr_t txr1, txr2;

void setup(void) {
    pvr_poly_cxt_t cxt;
    int i;
    float x, y, z;
    int w1, h1, w2, h2;
    uint32_t fmt1, fmt2;
    kos_img_t img;

    if(pcx_to_img("/rd/crate.pcx", &img)) {
        printf("Failed to load /rd/crate.pcx\n");
        exit(1);
    }

    w1 = img.w;
    h1 = img.h;
    fmt1 = PVR_TXRFMT_RGB565 | PVR_TXRFMT_TWIDDLED;
    txr1 = pvr_mem_malloc(img.byte_count);
    pvr_txr_load_kimg(&img, txr1, 0);
    kos_img_free(&img, 0);

    if(kmg_to_img("/rd/fruit.kmg", &img)) {
        printf("Failed to load /rd/fruit.kmg\n");
        exit(1);
    }

    w2 = img.w;
    h2 = img.h;
    fmt2 = PVR_TXRFMT_RGB565 | PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_TWIDDLED;
    txr2 = pvr_mem_malloc(img.byte_count);
    pvr_txr_load_kimg(&img, txr2, 0);
    kos_img_free(&img, 0);

    printf("Loaded textures\n");

    pvr_poly_cxt_txr_mod(&cxt, list, fmt1, w1, h1, txr1, PVR_FILTER_BILINEAR,
                         fmt2, w2, h2, txr2, PVR_FILTER_NONE);
    pvr_poly_mod_compile(&phdr, &cxt);

    pvr_mod_compile(&mhdr, list + 1, PVR_MODIFIER_OTHER_POLY, PVR_CULLING_NONE);
    pvr_mod_compile(&mhdr2, list + 1, PVR_MODIFIER_INCLUDE_LAST_POLY,
                    PVR_CULLING_NONE);

    for(i = 0; i < NUM_POLYS; ++i) {
        x = rand() % 640;
        y = rand() % 480;
        z = rand() % 100 + 1;

        verts[i * 4].flags = PVR_CMD_VERTEX;
        verts[i * 4].x = x - 50;
        verts[i * 4].y = y + 50;
        verts[i * 4].z = z;
        verts[i * 4].u0 = 0.0f;
        verts[i * 4].v0 = 1.0f;
        verts[i * 4].argb0 = 0xFFFFFFFF;
        verts[i * 4].oargb0 = 0xFF000000;
        verts[i * 4].u1 = 0.0f;
        verts[i * 4].v1 = 1.0f;
        verts[i * 4].argb1 = 0xFFFFFFFF;
        verts[i * 4].oargb1 = 0xFF000000;
        verts[i * 4].d1 = verts[i * 4].d2 = verts[i * 4].d3 =
                                                verts[i * 4].d4 = 0;

        verts[i * 4 + 1].flags = PVR_CMD_VERTEX;
        verts[i * 4 + 1].x = x - 50;
        verts[i * 4 + 1].y = y - 50;
        verts[i * 4 + 1].z = z;
        verts[i * 4 + 1].u0 = 0.0f;
        verts[i * 4 + 1].v0 = 0.0f;
        verts[i * 4 + 1].argb0 = 0xFFFFFFFF;
        verts[i * 4 + 1].oargb0 = 0xFF000000;
        verts[i * 4 + 1].u1 = 0.0f;
        verts[i * 4 + 1].v1 = 0.0f;
        verts[i * 4 + 1].argb1 = 0xFFFFFFFF;
        verts[i * 4 + 1].oargb1 = 0xFF000000;
        verts[i * 4 + 1].d1 = verts[i * 4 + 1].d2 = verts[i * 4 + 1].d3 =
                                  verts[i * 4 + 1].d4 = 0;

        verts[i * 4 + 2].flags = PVR_CMD_VERTEX;
        verts[i * 4 + 2].x = x + 50;
        verts[i * 4 + 2].y = y + 50;
        verts[i * 4 + 2].z = z;
        verts[i * 4 + 2].u0 = 1.0f;
        verts[i * 4 + 2].v0 = 1.0f;
        verts[i * 4 + 2].argb0 = 0xFFFFFFFF;
        verts[i * 4 + 2].oargb0 = 0xFF000000;
        verts[i * 4 + 2].u1 = 1.0f;
        verts[i * 4 + 2].v1 = 1.0f;
        verts[i * 4 + 2].argb1 = 0xFFFFFFFF;
        verts[i * 4 + 2].oargb1 = 0xFF000000;
        verts[i * 4 + 2].d1 = verts[i * 4 + 2].d2 = verts[i * 4 + 2].d3 =
                                  verts[i * 4 + 2].d4 = 0;

        verts[i * 4 + 3].flags = PVR_CMD_VERTEX_EOL;
        verts[i * 4 + 3].x = x + 50;
        verts[i * 4 + 3].y = y - 50;
        verts[i * 4 + 3].z = z;
        verts[i * 4 + 3].u0 = 1.0f;
        verts[i * 4 + 3].v0 = 0.0f;
        verts[i * 4 + 3].argb0 = 0xFFFFFFFF;
        verts[i * 4 + 3].oargb0 = 0xFF000000;
        verts[i * 4 + 3].u1 = 1.0f;
        verts[i * 4 + 3].v1 = 0.0f;
        verts[i * 4 + 3].argb1 = 0xFFFFFFFF;
        verts[i * 4 + 3].oargb1 = 0xFF000000;
        verts[i * 4 + 3].d1 = verts[i * 4 + 3].d2 = verts[i * 4 + 3].d3 =
                                  verts[i * 4 + 3].d4 = 0;
    }
}

int check_start(void) {
    maple_device_t *cont;
    cont_state_t *state;

    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if(cont) {
        state = (cont_state_t *)maple_dev_status(cont);

        if(!state)
            return 0;

        if(state->buttons & CONT_START)
            return 1;

        if(state->buttons & CONT_DPAD_UP)
            my -= 1.0f;

        if(state->buttons & CONT_DPAD_DOWN)
            my += 1.0f;

        if(state->buttons & CONT_DPAD_LEFT)
            mx -= 1.0f;

        if(state->buttons & CONT_DPAD_RIGHT)
            mx += 1.0f;
    }

    return 0;
}

void do_frame(void) {
    pvr_modifier_vol_t mod;
    int i;

    pvr_wait_ready();
    pvr_scene_begin();
    pvr_list_begin(list);

    pvr_prim(&phdr, sizeof(phdr));

    for(i = 0; i < NUM_POLYS; ++i) {
        pvr_prim(&verts[i * 4], sizeof(verts[i * 4]));
        pvr_prim(&verts[i * 4 + 1], sizeof(verts[i * 4 + 1]));
        pvr_prim(&verts[i * 4 + 2], sizeof(verts[i * 4 + 2]));
        pvr_prim(&verts[i * 4 + 3], sizeof(verts[i * 4 + 3]));
    }

    pvr_list_finish();

    pvr_list_begin(list + 1);

    pvr_prim(&mhdr, sizeof(mhdr));

    mod.flags = PVR_CMD_VERTEX_EOL;
    mod.ax = mx;
    mod.ay = my + 50.0f;
    mod.az = 150.0f;
    mod.bx = mx;
    mod.by = my;
    mod.bz = 150.0f;
    mod.cx = mx + 50.0f;
    mod.cy = my + 50.0f;
    mod.cz = 150.0f;
    mod.d1 = mod.d2 = mod.d3 = mod.d4 = mod.d5 = mod.d6 = 0;
    pvr_prim(&mod, sizeof(mod));

    pvr_prim(&mhdr2, sizeof(mhdr2));

    mod.flags = PVR_CMD_VERTEX_EOL;
    mod.ax = mx;
    mod.ay = my;
    mod.az = 150.0f;
    mod.bx = mx + 50.0f;
    mod.by = my + 50.0f;
    mod.bz = 150.0f;
    mod.cx = mx + 50.0f;
    mod.cy = my;
    mod.cz = 150.0f;
    mod.d1 = mod.d2 = mod.d3 = mod.d4 = mod.d5 = mod.d6 = 0;
    pvr_prim(&mod, sizeof(mod));

    pvr_list_finish();
    pvr_scene_finish();
}

static pvr_init_params_t pvr_params = {
    /* Enable Opaque and Opaque modifiers. */
    {
        PVR_BINSIZE_16, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0,
        PVR_BINSIZE_0
    },
    512 * 1024, 0, 0, 0, 0, 0
};

int main(int argc, char *argv[]) {
    printf("---KallistiOS PVR Modifier Example (with textures)---\n");
    printf("Use the DPAD to move the modifier square around (it starts at ");
    printf("(320, 240))\n");
    printf("Press Start to exit.\n");

    srand(time(NULL));

    pvr_init(&pvr_params);

    setup();

    /* Go as long as the user hasn't pressed start on controller 1. */
    while(!check_start())   {
        do_frame();
    }

    pvr_mem_free(txr1);
    pvr_mem_free(txr2);

    return 0;
}
