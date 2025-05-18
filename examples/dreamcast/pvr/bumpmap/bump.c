/* KallistiOS ##version##

   examples/dreamcast/pvr/bumpmap/bump.c
   Copyright (C) 2014 Lawrence Sebald

   This example demonstrates the use of bumpmaps on a surface.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include <dc/pvr.h>
#include <dc/maple.h>
#include <dc/fmath.h>
#include <dc/maple/controller.h>

#include <kos/init.h>

#include <kmg/kmg.h>

static pvr_sprite_txr_t sprites[2];

static pvr_sprite_hdr_t shdr[2];
static float bumpiness = 0.5f;
static pvr_ptr_t bump, txr;
static int textured = 1;

#define SIZE 256

#define CLAMP(low, high, value) ((value) < (low) ? (low) : ((value) > (high) ? (high) : (value)))

static pvr_ptr_t load_texture(const char fn[]) {
    FILE *fp;
    pvr_ptr_t rv;

    if(!(rv = pvr_mem_malloc(SIZE * SIZE * 2))) {
        printf("Failed to allocate PVR memory!\n");
        return NULL;
    }

    if(!(fp = fopen(fn, "rb"))) {
        printf("Couldn't open file: %s\n", strerror(errno));
        pvr_mem_free(rv);
        return NULL;
    }

    if(fread(rv, 1, SIZE * SIZE * 2, fp) != SIZE * SIZE * 2) {
        printf("Failed to read texture from file!\n");
        fclose(fp);
        pvr_mem_free(rv);
        return NULL;
    }

    fclose(fp);

    return rv;
}

static pvr_ptr_t load_kmg(const char fn[]) {
    kos_img_t img;
    pvr_ptr_t rv;

    if(kmg_to_img(fn, &img)) {
        printf("Failed to load image file: %s\n", fn);
        return NULL;
    }

    if(!(rv = pvr_mem_malloc(img.byte_count))) {
        printf("Couldn't allocate memory for texture!\n");
        kos_img_free(&img, 0);
        return NULL;
    }

    pvr_txr_load_kimg(&img, rv, 0);
    kos_img_free(&img, 0);

    return rv;
}

static void setup(void) {
    pvr_sprite_cxt_t cxt;

    /* Load the textures. */
    if(!(bump = load_texture("/rd/bumpmap.raw")))
        exit(EXIT_FAILURE);

    if(!(txr = load_kmg("/rd/bricks.kmg"))) {
        pvr_mem_free(bump);
        exit(EXIT_FAILURE);
    }

    /* Fill in the sprite context for the bumpmap itself. This is not for the
       polygon that will actually appear bumped. */
    pvr_sprite_cxt_txr(&cxt, PVR_LIST_OP_POLY,
                       PVR_TXRFMT_BUMP | PVR_TXRFMT_TWIDDLED, 256, 256, bump,
                       PVR_FILTER_NONE);
    cxt.gen.specular = PVR_SPECULAR_ENABLE;
    cxt.txr.env = PVR_TXRENV_DECAL;
    pvr_sprite_compile(&shdr[0], &cxt);

    /* Fill in the sprite context for the polygon to be "bumped" by the map. */
    pvr_sprite_cxt_txr(&cxt, PVR_LIST_PT_POLY,
                       PVR_TXRFMT_RGB565 | PVR_TXRFMT_TWIDDLED |
                       PVR_TXRFMT_VQ_ENABLE, 256, 256, txr, PVR_FILTER_NONE);
    cxt.blend.src = PVR_BLEND_DESTCOLOR;
    cxt.blend.dst = PVR_BLEND_ZERO;
    pvr_sprite_compile(&shdr[1], &cxt);

    /* Set up the two sprites. */
    sprites[0].flags = sprites[1].flags = PVR_CMD_VERTEX_EOL;
    sprites[0].ax = sprites[1].ax = 320.0f - 128.0f;
    sprites[0].ay = sprites[1].ay = 240.0f + 128.0f;
    sprites[0].az = sprites[1].az = 1.0f;
    sprites[0].bx = sprites[1].bx = 320.0f - 128.0f;
    sprites[0].by = sprites[1].by = 240.0f - 128.0f;
    sprites[0].bz = sprites[1].bz = 1.0f;
    sprites[0].cx = sprites[1].cx = 320.0f + 128.0f;
    sprites[0].cy = sprites[1].cy = 240.0f - 128.0f;
    sprites[0].cz = sprites[1].cz = 1.0f;
    sprites[0].dx = sprites[1].dx = 320.0f + 128.0f;
    sprites[0].dy = sprites[1].dy = 240.0f + 128.0f;
    sprites[0].dummy = sprites[1].dummy = 0;
    sprites[0].auv = sprites[1].auv = PVR_PACK_16BIT_UV(0.0f, 1.0f);
    sprites[0].buv = sprites[1].buv = PVR_PACK_16BIT_UV(0.0f, 0.0f);
    sprites[0].cuv = sprites[1].cuv = PVR_PACK_16BIT_UV(1.0f, 0.0f);
}

static void switch_textured(void) {
    pvr_sprite_cxt_t cxt;

    if(!textured)
        pvr_sprite_cxt_txr(&cxt, PVR_LIST_PT_POLY,
                           PVR_TXRFMT_RGB565 | PVR_TXRFMT_TWIDDLED |
                           PVR_TXRFMT_VQ_ENABLE, 256, 256, txr,
                           PVR_FILTER_NONE);
    else
        pvr_sprite_cxt_col(&cxt, PVR_LIST_PT_POLY);

    cxt.blend.src = PVR_BLEND_DESTCOLOR;
    cxt.blend.dst = PVR_BLEND_ZERO;
    pvr_sprite_compile(&shdr[1], &cxt);

    textured = !textured;
}

static int check_start(void) {
    maple_device_t *cont;
    cont_state_t *state;
    static int taken = 0;

    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if(cont) {
        state = (cont_state_t *)maple_dev_status(cont);

        if(!state)
            return 0;

        if(state->buttons & CONT_START)
            return 1;

        if(state->buttons & CONT_A) {
            if(!taken)
                switch_textured();

            taken = 1;
        }
        else {
            taken = 0;
        }

        if(state->joyy < -64)
            bumpiness = CLAMP(0.0f, 1.0f, bumpiness - 0.01f);
        else if(state->joyy > 64)
            bumpiness = CLAMP(0.0f, 1.0f, bumpiness + 0.01f);
    }

    return 0;
}

static void do_frame(void) {
    shdr[0].oargb = pvr_pack_bump(bumpiness, F_PI / 4.0f, 5.0f * F_PI / 6.0f);
    shdr[0].argb = 0;

    pvr_wait_ready();
    pvr_scene_begin();

    pvr_list_begin(PVR_LIST_OP_POLY);
    pvr_prim(&shdr[0], sizeof(pvr_sprite_hdr_t));
    pvr_prim(&sprites[0], sizeof(pvr_sprite_txr_t));
    pvr_list_finish();

    pvr_list_begin(PVR_LIST_PT_POLY);
    pvr_prim(&shdr[1], sizeof(pvr_sprite_hdr_t));
    pvr_prim(&sprites[1], sizeof(pvr_sprite_txr_t));
    pvr_list_finish();

    pvr_scene_finish();
}

static pvr_init_params_t pvr_params = {
    /* Enable only opaque and punchthru polygons. */
    {
        PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0, PVR_BINSIZE_0,
        PVR_BINSIZE_16
    },
    512 * 1024, 0, 0, 0, 0, 0
};

int main(int argc, char *argv[]) {
    printf("---KallistiOS PVR Bumpmap Example---\n");
    printf("Press A to switch between textured and non-textured mode.\n");
    printf("Use up and down on the joystick to control the bumpiness.\n");
    printf("Press Start to exit.\n");

    srand(time(NULL));

    pvr_init(&pvr_params);

    setup();

    /* Go as long as the user hasn't pressed start on controller 1. */
    while(!check_start()) {
        do_frame();
    }

    pvr_mem_free(bump);
    pvr_mem_free(txr);

    return 0;
}
