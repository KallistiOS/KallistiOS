/* This program is a slight modification of the libdream/ta example program.
   The big difference in this program is that we add in a small bit of user
   input code, and that we support render-to-texture mode. */

#include <stdio.h>

#include <arch/types.h>
#include <arch/timer.h>

#include <dc/pvr.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

/* A little test program -- creates six rainbow polygons and
   moves them around over a white background. */
typedef struct {
    float   x, y, z;
    float   dx, dy;
} polyplace_t;

polyplace_t polys[6] = {
    { 16.0f, 0.0f, 10.0f, 3.0f, 5.0f },
    { 624.0f, 0.0f, 12.0f, -3.0f, 7.0f },
    { 24.0f, 300.0f, 13.0f, 3.0f, -7.0f },
    { 0.0f, 16.0f, 16.0f, 5.0f, 3.0f },
    { 0.0f, 364.0f, 18.0f, 7.0f, -3.0f },
    { 480.0f, 24.0f, 19.0f, -7.0f, 3.0f },
};

/* Does one frame of polygon movement */
void move_polys(void) {
    int i;
    polyplace_t *p;

    for(i = 0; i < 6; i++) {
        p = polys + i;

        p->x += p->dx;

        if(p->x < 0 || p->x > 580.0f) {
            p->dx = -p->dx;
            p->x += p->dx;
        }

        p->y += p->dy;

        if(p->y < 0 || p->y > 420.0f) {
            p->dy = -p->dy;
            p->y += p->dy;
        }
    }
}

/* Sends one polygon's worth of data to the TA */
void draw_one_poly(polyplace_t *p) {
    /* Opaque Colored vertex */
    pvr_vertex_t vert;

    vert.flags = PVR_CMD_VERTEX;
    vert.x = p->x + 0.0f;
    vert.y = p->y + 240.0f;
    vert.z = p->z;
    vert.u = vert.v = 0.0f;
    vert.argb = 0xff0000ff;
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));

    vert.y = p->y + 0.0f;
    vert.argb = 0xff00ff00;
    pvr_prim(&vert, sizeof(vert));

    vert.x = p->x + 320.0f;
    vert.y = p->y + 240.0f;
    vert.argb = 0xff000000;
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.y = p->y + 0.0f;
    vert.argb = 0xffff0000;
    pvr_prim(&vert, sizeof(vert));
}

/* Sends one polygon's worth of data to the TA */
void draw_one_textured_poly(polyplace_t *p) {
    /* Opaque Textured vertex */
    pvr_vertex_t vert;

    vert.flags = PVR_CMD_VERTEX;
    vert.x = p->x + 0.0f;
    vert.y = p->y + 240.0f;
    vert.z = p->z;
    vert.u = 0.0f;
    vert.v = 480.0f / 512.0f;
    vert.argb = 0xffffffff;
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));

    vert.y = p->y + 0.0f;
    vert.v = 0.0f;
    pvr_prim(&vert, sizeof(vert));

    vert.x = p->x + 320.0f;
    vert.y = p->y + 240.0f;
    vert.u = 640.0f / 1024.0f;
    vert.v = 480.0f / 512.0f;
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.y = p->y + 0.0f;
    vert.v = 0.0f;
    pvr_prim(&vert, sizeof(vert));
}

int to_texture = 1;
pvr_ptr_t d_texture;
uint32_t tx_x = 1024, tx_y = 512;

void draw_frame(void) {
    pvr_poly_cxt_t  cxt;
    pvr_poly_hdr_t  poly;
    int i;

    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
    pvr_poly_compile(&poly, &cxt);

    /* Start opaque poly list */
    pvr_wait_ready();

    if(!to_texture)
        pvr_scene_begin();
    else
        pvr_scene_begin_txr(d_texture, &tx_x, &tx_y);

    pvr_list_begin(PVR_LIST_OP_POLY);

    /* Send polygon header to the TA using store queues */
    pvr_prim(&poly, sizeof(poly));

    /* Draw all polygons */
    for(i = 0; i < 6; i++)
        draw_one_poly(polys + i);

    /* End of opaque list */
    pvr_list_finish();

    /* Finish the frame */
    pvr_scene_finish();

    /* Move all polygons */
    if(!to_texture)
        move_polys();
}

void draw_textured(void) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    int i;

    draw_frame();

    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY,
                     PVR_TXRFMT_RGB565 | PVR_TXRFMT_NONTWIDDLED, 1024, 512,
                     d_texture, PVR_FILTER_NONE);
    pvr_poly_compile(&hdr, &cxt);

    pvr_wait_ready();
    pvr_scene_begin();

    /* Start opaque poly list */
    pvr_list_begin(PVR_LIST_OP_POLY);

    /* Send polygon header to the TA using store queues */
    pvr_prim(&hdr, sizeof(hdr));

    /* Draw all polygons */
    for(i = 0; i < 6; i++)
        draw_one_textured_poly(polys + i);

    /* End of opaque list */
    pvr_list_finish();

    /* Finish the frame */
    pvr_scene_finish();

    /* Move all polygons */
    move_polys();
}

/* Main program: init and loop drawing polygons */
pvr_init_params_t pvr_params = {
    { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0,
      PVR_BINSIZE_0, PVR_BINSIZE_0 },
    64 * 1024, 0, 0, 0, 0, 0
};

int main(int argc, char **argv) {
    maple_device_t *cont;
    cont_state_t *state;
    int finished = 0;
    uint64_t timer = timer_ms_gettime64(), start, end;
    uint32_t counter = 0;
    pvr_stats_t stats;

    pvr_init(&pvr_params);

    /* Allocate our texture to be rendered to */
    d_texture = pvr_mem_malloc(1024 * 512 * 2);

    pvr_set_bg_color(1.0f, 1.0f, 1.0f);

    start = timer_ms_gettime64();

    while(!finished) {
        cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

        if(cont != NULL) {
            state = (cont_state_t *) maple_dev_status(cont);

            if(state != NULL && timer < timer_ms_gettime64()) {
                if(state->buttons & CONT_START)
                    finished = 1;
                else if(state->buttons & CONT_A && (to_texture % 2) != 1) {
                    ++to_texture;
                    timer = timer_ms_gettime64() + 200;
                }
                else if(state->buttons & CONT_B && to_texture) {
                    to_texture = 0;
                    timer = timer_ms_gettime64() + 200;
                }
            }
        }

        if(!to_texture) {
            draw_frame();
            ++counter;
        }
        else {
            draw_textured();
            counter += 2;
        }
    }

    end = timer_ms_gettime64();

    printf("%lu frames in %llu ms = %f FPS\n", counter, end - start,
           (double)(counter / ((float)end - start) * 1000.0f));

    pvr_get_stats(&stats);
    printf("From pvr_get_stats:\n\tVBlank Count: %u\n\tFrame Count: %u\n",
           stats.vbl_count, stats.frame_count);

    pvr_mem_free(d_texture);

    /* Shutdown isn't technically necessary, but is possible */
    pvr_shutdown();

    return 0;
}

