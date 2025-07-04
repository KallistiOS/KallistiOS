/* Simple program that shows how the back buffer (the one being currently
 * presented to the screen) can be used as a texture when rendering to the
 * front buffer. */
/* Copyright (C) 2025 Paul Cercueil */

#include <kos.h>
#include <stdio.h>
#include <stdbool.h>

/* XXX: KallistiOS has the wrong value for the stride bit. */
#undef PVR_TXRFMT_STRIDE
#define PVR_TXRFMT_STRIDE BIT(25)

#define SQUARE_SIZE 64

#define FRONT_RENDER_COLOR 0xfff8f8f8

struct square_fcoords {
    float x[4];
    float y[4];
    float u[4];
    float v[4];
};

static bool done;

static uint32_t dr_state;

static void do_exit(uint8_t, uint32_t)
{
    done = 1;
}

alignas(4) static const uint16_t fake_tex_data[] = {
    /* Alternating 0x8000 / 0x0000 but pre-twiddled */
    0x8000, 0x8000, 0x0000, 0x0000, 0x8000, 0x8000, 0x0000, 0x0000,
    0x8000, 0x8000, 0x0000, 0x0000, 0x8000, 0x8000, 0x0000, 0x0000,
    0x8000, 0x8000, 0x0000, 0x0000, 0x8000, 0x8000, 0x0000, 0x0000,
    0x8000, 0x8000, 0x0000, 0x0000, 0x8000, 0x8000, 0x0000, 0x0000,
    0x8000, 0x8000, 0x0000, 0x0000, 0x8000, 0x8000, 0x0000, 0x0000,
    0x8000, 0x8000, 0x0000, 0x0000, 0x8000, 0x8000, 0x0000, 0x0000,
    0x8000, 0x8000, 0x0000, 0x0000, 0x8000, 0x8000, 0x0000, 0x0000,
    0x8000, 0x8000, 0x0000, 0x0000, 0x8000, 0x8000, 0x0000, 0x0000,
};

static void render_coords(const struct square_fcoords *coords, float z, uint32_t argb) {
    pvr_vertex_t *vert;
    unsigned int i;

    for(i = 0; i < 4; i++) {
        vert = pvr_dr_target(dr_state);
        *vert = (pvr_vertex_t){
            .flags = i == 3 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX,
            .x = coords->x[i],
            .y = coords->y[i],
            .z = z,
            .u = coords->u[i],
            .v = coords->v[i],
            .argb = argb,
        };
        pvr_dr_commit(vert);
    }
}

static void render_bouncing_cube(uint16_t x, uint16_t y) {
    pvr_poly_hdr_t *hdr_sq;
    pvr_poly_cxt_t cxt;
    const struct square_fcoords coords = {
        .x = { x, x + SQUARE_SIZE, x, x + SQUARE_SIZE },
        .y = { y, y, y + SQUARE_SIZE, y + SQUARE_SIZE },
    };
    static uint32_t color = 0xffff0000;
    static uint32_t cnt = 0;

    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);

    hdr_sq = (void *)pvr_dr_target(dr_state);
    pvr_poly_compile(hdr_sq, &cxt);
    pvr_dr_commit(hdr_sq);

    if (cnt % 256) {
        switch (cnt / 256) {
        case 0:
            color += 1 << 8;
            break;
        case 1:
            color -= 1 << 16;
            break;
        case 2:
            color += 1;
            break;
        case 3:
            color -= 1 << 8;
            break;
        case 4:
            color += 1 << 16;
            break;
        case 5:
            color -= 1;
            break;
        }
    }

    render_coords(&coords, 4.0f, color);

    cnt++;
    if (cnt == 6 * 256)
        cnt = 0;
}

static void render_back_buffer_step1(pvr_ptr_t fake_tex) {
    pvr_poly_hdr_t *hdr_sq;
    pvr_poly_cxt_t cxt;
    struct square_fcoords coords = {
        .x = { 0.0f, 640.0f, 0.0f, 640.0f },
        .y = { 0.0f, 0.0f, 480.0f, 480.0f },
        .u = { 0.0f, 640.0f / 8.0f, 0.0f, 640.0f / 8.0f },
        .v = { 0.0f, 0.0f, 480.0f / 8.0f, 480.0f / 8.0f },
    };

    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_ARGB1555,
                     8, 8, fake_tex, PVR_FILTER_NEAREST);

    cxt.txr.alpha = PVR_TXRALPHA_ENABLE;

    hdr_sq = (void *)pvr_dr_target(dr_state);
    pvr_poly_compile(hdr_sq, &cxt);
    pvr_dr_commit(hdr_sq);

    render_coords(&coords, 1.0f, 0xffffffff);
}

static void render_back_buffer_step2(pvr_ptr_t frontbuf) {
    pvr_poly_hdr_t *hdr_sq;
    pvr_poly_cxt_t cxt;
    struct square_fcoords coords_lefthalf = {
        .x = { 0.0f, 320.0f, 0.0f, 320.0f },
        .y = { 0.0f, 0.0f, 480.0f, 480.0f },
        .u = { 0.0f, 640.0f / 1024.0f, 0.0f, 640.0f / 1024.0f },
        .v = { 0.0f, 0.0f, 960.0f / 1024.0f, 960.0f / 1024.0f },
    };
    struct square_fcoords coords_righthalf = {
        .x = { 320.0f, 640.0f, 320.0f, 640.0f },
        .y = { 0.0f, 0.0f, 480.0f, 480.0f },
        .u = { 0.0f, 640.0f / 1024.0f, 0.0f, 640.0f / 1024.0f },
        .v = { 1.0f / 1024.0f, 1.0f / 1024.0f, 961.0f / 1024.0f, 961.0f / 1024.0f },
    };

    pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY,
                     PVR_TXRFMT_RGB565 | PVR_TXRFMT_NONTWIDDLED | PVR_TXRFMT_STRIDE,
                     1024, 1024, frontbuf, PVR_FILTER_NEAREST);

    cxt.txr.alpha = PVR_TXRALPHA_DISABLE;
    cxt.blend.dst = PVR_BLEND_ZERO;
    cxt.blend.src = PVR_BLEND_DESTALPHA;

    hdr_sq = (void *)pvr_dr_target(dr_state);
    pvr_poly_compile(hdr_sq, &cxt);
    pvr_dr_commit(hdr_sq);

    /* We have to render the left and right parts separately.
     * This is because our framebuffer texture has a stride of 1280 pixels
     * (640 real pixels + 640 garbage pixels), and the PVR supports a texture
     * size of 1024x1024 max.
     * So we just lie and tell the PVR it's a 640x960 texture, which we squash
     * to two 320x480 sides, and craft the U/V coordinates so that each second
     * pixel is skipped, and each second line is skipped. Adding 1.0f / 1024.0f
     * to the V coordinate can then select the right side of the image instead
     * of the left side.
     */
    render_coords(&coords_lefthalf, 2.0f, FRONT_RENDER_COLOR);
    render_coords(&coords_righthalf, 2.0f, FRONT_RENDER_COLOR);
}

static void render_back_buffer_step3(pvr_ptr_t frontbuf) {
    pvr_poly_hdr_t *hdr_sq;
    pvr_poly_cxt_t cxt;
    struct square_fcoords coords_lefthalf = {
        .x = { 0.0f, 320.0f, 0.0f, 320.0f },
        .y = { 0.0f, 0.0f, 480.0f, 480.0f },
        .u = { -1.0f / 1024.0f, 639.0f / 1024.0f, -1.0f / 1024.0f, 639.0f / 1024.0f },
        .v = { 0.0f, 0.0f, 960.0f / 1024.0f, 960.0f / 1024.0f },
    };
    struct square_fcoords coords_righthalf = {
        .x = { 320.0f, 640.0f, 320.0f, 640.0f },
        .y = { 0.0f, 0.0f, 480.0f, 480.0f },
        .u = { -1.0f / 1024.0f, 639.0f / 1024.0f, -1.0f / 1024.0f, 639.0f / 1024.0f },
        .v = { 1.0f / 1024.0f, 1.0f / 1024.0f, 961.0f / 1024.0f, 961.0f / 1024.0f },
    };

    pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY,
                     PVR_TXRFMT_RGB565 | PVR_TXRFMT_NONTWIDDLED | PVR_TXRFMT_STRIDE,
                     1024, 1024, frontbuf, PVR_FILTER_NEAREST);

    cxt.txr.alpha = PVR_TXRALPHA_DISABLE;
    cxt.blend.dst = PVR_BLEND_ONE;
    cxt.blend.src = PVR_BLEND_INVDESTALPHA;

    hdr_sq = (void *)pvr_dr_target(dr_state);
    pvr_poly_compile(hdr_sq, &cxt);
    pvr_dr_commit(hdr_sq);

    render_coords(&coords_lefthalf, 3.0f, FRONT_RENDER_COLOR);
    render_coords(&coords_righthalf, 3.0f, FRONT_RENDER_COLOR);
}

int main(int argc, char **argv) {
    unsigned int x = 0, y = 0;
    bool xneg = false, yneg = false;
    pvr_ptr_t fake_tex, frontbuf;

    vid_set_mode(DM_640x480, PM_RGB565);

    /* Force the video mode without dithering */
    PVR_SET(PVR_FB_CFG_2, 1 /* PVR_PM_RGB565 */);

    pvr_init_defaults();

    /* Set the stride length for strided textures.
     * Unit is a set of 32 pixels, hence the /32. */
    PVR_SET(PVR_TEXTURE_MODULO, 640 / 32);

    fake_tex = pvr_mem_malloc(sizeof(fake_tex_data));
    pvr_txr_load(fake_tex_data, fake_tex, sizeof(fake_tex_data));

    cont_btn_callback(0, CONT_START, do_exit);

    while(!done) {
        pvr_scene_begin();
        frontbuf = pvr_get_front_buffer();

        pvr_list_begin(PVR_LIST_OP_POLY);

        /* In the background, render a argb1555 texture on the whole screen,
         * with pixels alternating between full-alpha black (0x8000), and
         * zero-alpha black (0x0000). This mask will permit to extract the
         * framebuffer texture properly.
         *
         * Note that we are rendering an opaque quad here, which means anything
         * behind the mask texture will be discarded. If that's not wanted
         * (e.g. when doing a "blur" effect where the framebuffer is applied on
         * top of the scene), it is possible to use full-alpha white (0xffff)
         * and zero-alpha white (0x7fff) instead, and blend it on top of the
         * scene with (blend_src = invdestcolor, blend_dst = zero). The quad
         * then obviously should go through the transparent list.
         *
         * Using the latter may be a good idea also when the goal is to re-use
         * the previous framebuffer as the background for the new scene, in
         * the case where the background is unlikely to be shown (or only
         * small parts are shown). As transparent geometry is rendered last,
         * if opaque geometry has already be rendered at higher Z values,
         * the PVR won't spend time rendering the mask texture.
         */
        render_back_buffer_step1(fake_tex);

        /* Render a bouncing cube in the foreground, with circling colors. */
        render_bouncing_cube(x, y);
        pvr_list_finish();

        pvr_list_begin(PVR_LIST_TR_POLY);

        /* The front buffer is in 32-bit memory, while textures are read from
         * 64-bit memory. Therefore if we try to use the front buffer as a
         * regular texture, we'll have two correct pixels followed by two
         * garbage pixels.
         *
         * To work around that, we squash the texture horizontally so that
         * every second pixel is skipped. Then, by crafting specific U/V values,
         * we can retrieve the odd pixels separated by garbage pixels. Using
         * different U/V values, we can retrieve the even pixels separated by
         * garbage pixels.
         *
         * To discard the garbage pixels, we first blend the odd pixels on top
         * of the mask texture, with blend_src = destalpha. Then, we blend the
         * even pixels on top of that, with blend_src = invdestalpha.
         *
         * Note that this trick only works because we render to a square that's
         * the size of the screen. This trick would not work if the texture had
         * to be scaled, or rotated.
         *
         * Also note that the blend for those two steps can be done with a
         * non-white color (as it is the case here, with FRONT_RENDER_COLOR).
         * This is useful for instance when emulating blur. However, it should
         * be noted that when dithering is enabled, some unwanted artifacts
         * can appear, as the frame will never fully fade to black.
         * For that reason, we disabled dithering after setting the video mode.
         */
        render_back_buffer_step2(frontbuf);
        render_back_buffer_step3(frontbuf);

        pvr_list_finish();
        pvr_scene_finish();

        /* Handle the square bouncing around */
        if(xneg)
            x--;
        else
            x++;
        if(yneg)
            y--;
        else
            y++;

        if(x == 0 || x == 640 - SQUARE_SIZE)
            xneg = !xneg;
        if(y == 0 || y == 480 - SQUARE_SIZE)
            yneg = !yneg;
    }

    pvr_mem_free(fake_tex);

    return 0;
}
