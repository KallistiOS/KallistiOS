/*
   pvr_rtt_sized
   Copyright (C) 2026 Troy Davis

   Demonstrates pvr_scene_begin_rtt() with a small user-sized RTT texture.

   The older pvr_scene_begin_txr() examples commonly use a 1024x512 RGB565
   backing texture:

      1024 * 512 * 2 = 1048576 bytes = 1 MiB

   By default, this example renders directly into a 128x128 RGB565 texture:

      128 * 128 * 2 = 32768 bytes = 32 KiB

   Change SMALL_RTT_SIZE to 64 to test an 8 KiB square target:

      64 * 64 * 2 = 8192 bytes = 8 KiB

   Override SMALL_RTT_W, SMALL_RTT_H, and SMALL_RTT_STRIDE_PX to test a
   non-square target, such as 128x64 with a 128-pixel stride.

   Controls:
      LEFT   Previous RTT mode
      RIGHT  Next RTT mode
      START  Exit
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/pvr.h>
#include <dcplib/fnt.h>

#ifndef SMALL_RTT_SIZE
#define SMALL_RTT_SIZE 128
#endif

#ifndef SMALL_RTT_W
#define SMALL_RTT_W SMALL_RTT_SIZE
#endif

#ifndef SMALL_RTT_H
#define SMALL_RTT_H SMALL_RTT_SIZE
#endif

#ifndef SMALL_RTT_STRIDE_PX
#define SMALL_RTT_STRIDE_PX SMALL_RTT_W
#endif

#define RGB565_BYTES 2
#define OLD_RTT_BYTES (1024 * 512 * RGB565_BYTES)

typedef struct rtt_mode {
    uint32_t w;
    uint32_t h;
    uint32_t stride_px;
} rtt_mode_t;

static const rtt_mode_t rtt_modes[] = {
    { 64, 64, 64 },
    { SMALL_RTT_W, SMALL_RTT_H, SMALL_RTT_STRIDE_PX },
    { 128, 64, 128 }
};

#define RTT_MODE_COUNT ((int)(sizeof(rtt_modes) / sizeof(rtt_modes[0])))

static pvr_ptr_t rtt_texture;
static fntRenderer *text;
static fntTexFont *font;
static int rtt_mode_index = 1;
static float marker_x = 8.0f;
static float marker_y = 18.0f;
static float marker_dx = 1.0f;
static float marker_dy = 1.5f;
static bool exit_requested;
static uint32_t last_buttons;

static pvr_init_params_t pvr_params = {
    { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16,
      PVR_BINSIZE_0, PVR_BINSIZE_0 },
    256 * 1024, 0, 0, 0, 0, 0
};

static float clampf(float value, float min, float max) {
    if(max < min)
        max = min;

    if(value < min)
        return min;
    if(value > max)
        return max;

    return value;
}

static const rtt_mode_t *current_rtt_mode(void) {
    return rtt_modes + rtt_mode_index;
}

static uint32_t current_rtt_bytes(void) {
    const rtt_mode_t *mode = current_rtt_mode();

    return mode->stride_px * mode->h * RGB565_BYTES;
}

static uint32_t rtt_mode_bytes(const rtt_mode_t *mode) {
    return mode->stride_px * mode->h * RGB565_BYTES;
}

static void clamp_marker_to_rtt(void) {
    const rtt_mode_t *mode = current_rtt_mode();
    float max_x = (float)mode->w - 28.0f;
    float max_y = (float)mode->h - 28.0f;

    marker_x = clampf(marker_x, 4.0f, max_x);
    marker_y = clampf(marker_y, 4.0f, max_y);
}

static int alloc_rtt_texture(void) {
    uint32_t bytes = current_rtt_bytes();
    pvr_ptr_t txr = pvr_mem_malloc(bytes);

    if(!txr)
        return -1;

    if(rtt_texture)
        pvr_mem_free(rtt_texture);

    rtt_texture = txr;

    printf("RTT mode: %lux%lu stride=%lu bytes=%lu\n",
           (unsigned long)current_rtt_mode()->w,
           (unsigned long)current_rtt_mode()->h,
           (unsigned long)current_rtt_mode()->stride_px,
           (unsigned long)bytes);

    return 0;
}

static void change_rtt_mode(int direction) {
    int new_mode = (rtt_mode_index + direction + RTT_MODE_COUNT) %
                   RTT_MODE_COUNT;

    /* Texture lifetime changes need explicit completion: the previous screen
       scene may still be rendering and sampling from the old RTT texture. */
    pvr_wait_ready();
    pvr_wait_render_done();

    rtt_mode_index = new_mode;
    clamp_marker_to_rtt();

    if(alloc_rtt_texture() < 0)
        exit_requested = true;
}

static void submit_colored_rect(float x, float y, float w, float h, float z,
                                uint32_t argb) {
    pvr_vertex_t vert;

    vert.flags = PVR_CMD_VERTEX;
    vert.x = x;
    vert.y = y + h;
    vert.z = z;
    vert.u = 0.0f;
    vert.v = 0.0f;
    vert.argb = argb;
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));

    vert.y = y;
    pvr_prim(&vert, sizeof(vert));

    vert.x = x + w;
    vert.y = y + h;
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.y = y;
    pvr_prim(&vert, sizeof(vert));
}

static void submit_textured_rect(float x, float y, float w, float h, float z,
                                 float u1, float v1) {
    pvr_vertex_t vert;

    vert.flags = PVR_CMD_VERTEX;
    vert.x = x;
    vert.y = y + h;
    vert.z = z;
    vert.u = 0.0f;
    vert.v = v1;
    vert.argb = 0xffffffff;
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));

    vert.y = y;
    vert.v = 0.0f;
    pvr_prim(&vert, sizeof(vert));

    vert.x = x + w;
    vert.y = y + h;
    vert.u = u1;
    vert.v = v1;
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.y = y;
    vert.v = 0.0f;
    pvr_prim(&vert, sizeof(vert));
}

static void update_motion(void) {
    const rtt_mode_t *mode = current_rtt_mode();
    float max_x = (float)mode->w - 28.0f;
    float max_y = (float)mode->h - 28.0f;

    if(max_x < 4.0f)
        max_x = 4.0f;
    if(max_y < 4.0f)
        max_y = 4.0f;

    marker_x += marker_dx;
    marker_y += marker_dy;

    if(marker_x < 4.0f || marker_x > max_x) {
        marker_dx = -marker_dx;
        marker_x += marker_dx;
    }

    if(marker_y < 4.0f || marker_y > max_y) {
        marker_dy = -marker_dy;
        marker_y += marker_dy;
    }

    clamp_marker_to_rtt();
}

static void render_rtt_scene(void) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    const rtt_mode_t *mode = current_rtt_mode();
    float green_x, green_y;

    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
    pvr_poly_compile(&hdr, &cxt);

    if(pvr_scene_begin_rtt(rtt_texture, mode->w, mode->h,
                           mode->stride_px) < 0) {
        printf("pvr_scene_begin_rtt failed\n");
        exit_requested = true;
        return;
    }

    pvr_list_begin(PVR_LIST_OP_POLY);
    pvr_prim(&hdr, sizeof(hdr));

    submit_colored_rect(0.0f, 0.0f, mode->w, mode->h, 1.0f,
                        0xff102040);
    submit_colored_rect(8.0f, 8.0f, mode->w - 16.0f,
                        mode->h - 16.0f, 2.0f, 0xff204060);
    submit_colored_rect(marker_x, marker_y, 24.0f, 24.0f, 3.0f, 0xffff3030);
    green_x = clampf((float)mode->w - marker_y - 24.0f, 4.0f,
                     (float)mode->w - 22.0f);
    green_y = clampf(marker_x, 4.0f, (float)mode->h - 22.0f);
    submit_colored_rect(green_x, green_y, 18.0f, 18.0f, 4.0f, 0xff30ff80);
    submit_colored_rect(0.0f, 0.0f, mode->w, 4.0f, 5.0f, 0xffffffff);
    submit_colored_rect(0.0f, mode->h - 4.0f, mode->w, 4.0f,
                        5.0f, 0xffffffff);
    submit_colored_rect(0.0f, 0.0f, 4.0f, mode->h, 5.0f, 0xffffffff);
    submit_colored_rect(mode->w - 4.0f, 0.0f, 4.0f, mode->h,
                        5.0f, 0xffffffff);

    pvr_list_finish();

    /* No pvr_wait_ready() / pvr_wait_render_done() is needed here. The next
       screen scene samples this texture on the PVR, and KOS orders render
       scenes so the screen pass cannot start rendering until this RTT pass is
       done. Explicit waits are only needed before CPU access or texture
       lifetime changes, such as freeing/reallocating the RTT texture. */
    pvr_scene_finish();
}

static void draw_status_text(void) {
    const rtt_mode_t *mode = current_rtt_mode();
    uint32_t bytes = current_rtt_bytes();
    char line[64];
    int y = 316;

    text->setFilterMode(PVR_FILTER_NEAREST);
    text->setFont(font);
    text->setPointSize(18);
    text->begin();
    text->setColor(1, 1, 1);

    snprintf(line, sizeof(line), "RTT: %lux%lu RGB565",
             (unsigned long)mode->w, (unsigned long)mode->h);
    text->start2f(32, y);
    text->puts(line);
    y += 22;

    snprintf(line, sizeof(line), "PVR RAM: %lu bytes / %lu KiB",
             (unsigned long)bytes, (unsigned long)(bytes / 1024));
    text->start2f(32, y);
    text->puts(line);
    y += 22;

    text->start2f(32, y);
    text->puts("LEFT/RIGHT: change RTT size");
    y += 22;

    text->start2f(32, y);
    text->puts("START: exit");
    text->end();
}

static void render_screen_scene(void) {
    pvr_poly_cxt_t txr_cxt;
    pvr_poly_cxt_t col_cxt;
    pvr_poly_hdr_t txr_hdr;
    pvr_poly_hdr_t col_hdr;
    const rtt_mode_t *mode = current_rtt_mode();
    float u1 = (float)mode->w / (float)mode->stride_px;
    float v1 = 1.0f;

    pvr_poly_cxt_txr(&txr_cxt, PVR_LIST_OP_POLY,
                     PVR_TXRFMT_RGB565 | PVR_TXRFMT_NONTWIDDLED,
                     mode->stride_px, mode->h, rtt_texture,
                     PVR_FILTER_NONE);
    pvr_poly_compile(&txr_hdr, &txr_cxt);

    pvr_poly_cxt_col(&col_cxt, PVR_LIST_OP_POLY);
    pvr_poly_compile(&col_hdr, &col_cxt);

    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_OP_POLY);

    pvr_prim(&txr_hdr, sizeof(txr_hdr));
    submit_textured_rect(56.0f, 56.0f, 192.0f, 192.0f, 1.0f, u1, v1);
    submit_textured_rect(300.0f, 80.0f, 96.0f, 96.0f, 1.0f, u1, v1);
    submit_textured_rect(420.0f, 132.0f, 144.0f, 144.0f, 1.0f, u1, v1);

    pvr_prim(&col_hdr, sizeof(col_hdr));
    submit_colored_rect(36.0f, 36.0f, 548.0f, 8.0f, 2.0f, 0xff30a0ff);
    submit_colored_rect(36.0f, 292.0f, 548.0f, 8.0f, 2.0f, 0xffffd030);

    pvr_list_finish();

    pvr_list_begin(PVR_LIST_TR_POLY);
    draw_status_text();
    pvr_list_finish();

    pvr_scene_finish();
}

static void poll_controls(void) {
    maple_device_t *cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
    cont_state_t *state;
    uint32_t pressed;

    if(!cont)
        return;

    state = (cont_state_t *)maple_dev_status(cont);
    if(!state)
        return;

    pressed = state->buttons & ~last_buttons;

    if(pressed & CONT_START)
        exit_requested = true;

    if(pressed & CONT_DPAD_LEFT) {
        change_rtt_mode(-1);
    }

    if(pressed & CONT_DPAD_RIGHT) {
        change_rtt_mode(1);
    }

    last_buttons = state->buttons;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    int i;

    pvr_init(&pvr_params);
    text = new fntRenderer();
    font = new fntTexFont("/rd/default.txf");

    if(alloc_rtt_texture() < 0) {
        printf("Failed to allocate %dx%d RGB565 RTT texture, stride=%d\n",
               (int)current_rtt_mode()->w, (int)current_rtt_mode()->h,
               (int)current_rtt_mode()->stride_px);
        delete font;
        delete text;
        pvr_shutdown();
        return EXIT_FAILURE;
    }

    printf("pvr_rtt_sized: pvr_scene_begin_rtt small RTT example\n");
    printf("old 1024x512 RGB565 RTT backing: %d bytes (1 MiB)\n",
           OLD_RTT_BYTES);
    printf("available RTT modes:\n");
    for(i = 0; i < RTT_MODE_COUNT; i++) {
        const rtt_mode_t *mode = rtt_modes + i;
        uint32_t bytes = rtt_mode_bytes(mode);

        printf("  %lux%lu stride=%lu RGB565: %lu bytes (%lu KiB)\n",
               (unsigned long)mode->w, (unsigned long)mode->h,
               (unsigned long)mode->stride_px, (unsigned long)bytes,
               (unsigned long)(bytes / 1024));
    }
    printf("controls: LEFT/RIGHT=size START=exit\n");

    pvr_set_bg_color(0.0f, 0.0f, 0.0f);
    last_buttons = 0;

    while(!exit_requested) {
        render_rtt_scene();
        if(!exit_requested)
            render_screen_scene();

        update_motion();
        poll_controls();
    }

    pvr_wait_ready();
    pvr_wait_render_done();
    pvr_mem_free(rtt_texture);
    delete font;
    delete text;
    pvr_shutdown();

    return EXIT_SUCCESS;
}
