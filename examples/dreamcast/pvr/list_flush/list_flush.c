/* KallistiOS ##version##

   examples/dreamcast/pvr/list_flush/list_flush.c
   Copyright (C) 2026 Daniel Fairchild

   Hybrid submission: one list collected in RAM and sent by DMA, the other
   submitted directly, with pvr_list_flush() sending the collected list
   ahead of the direct one.

   With vertex DMA, a list with a vertex buffer is collected in RAM and a
   list without one is submitted directly. Here the translucent particles
   are collected while the frame's state is worked out, and the opaque
   tiles behind them are submitted directly afterwards. pvr_list_flush()
   sends the particles to the TA as soon as they are built; without it,
   pvr_scene_finish() sends them after the tiles. The picture is the same
   either way. The TA can only take a direct list once it is done with the
   previous scene, and pvr_list_flush() waits for that too.

   It switches between the two every 5 seconds and prints, for each, the
   CPU time from pvr_scene_begin() to the return of pvr_scene_finish().
   Flushing early waits for the DMA inside the frame, so it costs the CPU
   the DMA time (about 0.2 ms for these 75 KB). In return the TA has the
   whole scene when pvr_scene_finish() returns, where otherwise the
   particles still have to be sent.

   Controls:
     A          flush the particles early / send them at the scene's end
     Start      quit
*/

#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include <arch/timer.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/pvr.h>

#define PARTICLES 600
/* Split in two, one half per scene: 600 particles of 4 vertices and 32
   bytes each take 75 KB. */
#define TR_BUF_SIZE (256 * 1024)

static uint8_t tr_buf[TR_BUF_SIZE] __attribute__((aligned(32)));
static pvr_poly_hdr_t op_hdr, tr_hdr;

static void quad(float x0, float y0, float x1, float y1, float z,
                 uint32_t argb) {
    pvr_vertex_t v[4] __attribute__((aligned(32)));
    int i;

    for(i = 0; i < 4; i++) {
        v[i].flags = i == 3 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
        v[i].x = (i & 1) ? x1 : x0;
        v[i].y = (i & 2) ? y0 : y1;
        v[i].z = z;
        v[i].u = v[i].v = 0.0f;
        v[i].argb = argb;
        v[i].oargb = 0;
    }

    pvr_prim(v, sizeof(v));
}

static void setup(void) {
    pvr_init_params_t params = {
        .opb_sizes = { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16,
                       PVR_BINSIZE_0, PVR_BINSIZE_0 },
        .vertex_buf_size = 512 * 1024,
        .dma_enabled = 1,
        .opb_overflow_count = 2,
    };
    pvr_poly_cxt_t cxt;

    pvr_init(&params);
    pvr_set_bg_color(0.0f, 0.0f, 0.0f);

    /* The translucent list is collected in RAM; the opaque list has no
       vertex buffer, so it is submitted directly. */
    pvr_set_vertbuf(PVR_LIST_TR_POLY, tr_buf, TR_BUF_SIZE);

    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
    pvr_poly_compile(&op_hdr, &cxt);
    pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
    cxt.depth.write = PVR_DEPTHWRITE_DISABLE;
    pvr_poly_compile(&tr_hdr, &cxt);
}

/* The particles: small squares on circles of their own. */
static void particles(float t) {
    float a, r, x, y;
    int i;

    pvr_list_begin(PVR_LIST_TR_POLY);
    pvr_prim(&tr_hdr, sizeof(tr_hdr));

    for(i = 0; i < PARTICLES; i++) {
        a = t * (0.3f + (i % 7) * 0.1f) + i * 2.399f;
        r = 20.0f + (i % 200) * 1.1f;
        x = 320.0f + cosf(a) * r;
        y = 240.0f + sinf(a) * r * 0.8f;
        quad(x - 6, y - 6, x + 6, y + 6, 10.0f + i * 0.01f,
             0x60000000 | ((i * 0x3b) & 0xff) << 16 | 0xa0ff);
    }

    pvr_list_finish();
}

/* The opaque tiles behind them. */
static void tiles(float t) {
    int x, y;
    uint32_t c;

    pvr_list_begin(PVR_LIST_OP_POLY);
    pvr_prim(&op_hdr, sizeof(op_hdr));

    for(y = 0; y < 480; y += 20) {
        for(x = 0; x < 640; x += 20) {
            c = (uint32_t)(96 + 64 * sinf(t + x * 0.02f + y * 0.03f));
            quad(x, y, x + 19, y + 19, 1.0f,
                 0xff000000 | c << 8 | (((x + y) / 20) & 1 ? 0x40 : 0x20));
        }
    }

    pvr_list_finish();
}

int main(int argc, char *argv[]) {
    maple_device_t *cont;
    cont_state_t *st;
    uint32_t buttons, prev = 0, pressed;
    uint64_t begin_us, cpu_us = 0, last_switch;
    bool early = true;
    float t = 0.0f;
    int frames = 0;

    (void)argc;
    (void)argv;

    setup();
    printf("list_flush: A = flush early / at the end, Start = exit\n");
    last_switch = timer_ms_gettime64();

    for(;;) {
        buttons = 0;

        if((cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER)) &&
           (st = (cont_state_t *)maple_dev_status(cont)))
            buttons = st->buttons;

        pressed = buttons & ~prev;
        prev = buttons;

        if(pressed & CONT_START)
            break;

        if((pressed & CONT_A) ||
           timer_ms_gettime64() - last_switch >= 5000) {
            if(frames)
                printf("list_flush: %s: CPU %.3f ms a frame\n",
                       early ? "flushed early" : "sent at the end",
                       cpu_us / 1000.0 / frames);

            early = !early;
            cpu_us = 0;
            frames = 0;
            last_switch = timer_ms_gettime64();
        }

        pvr_wait_ready();
        begin_us = timer_us_gettime64();
        pvr_scene_begin();

        particles(t);

        if(early)
            pvr_list_flush(PVR_LIST_TR_POLY);

        tiles(t);
        pvr_scene_finish();
        cpu_us += timer_us_gettime64() - begin_us;
        frames++;

        t += 1.0f / 60.0f;
    }

    pvr_wait_ready();
    pvr_shutdown();

    return 0;
}
