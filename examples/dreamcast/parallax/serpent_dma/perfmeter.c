/*
   perfmeter.c

   Copyright (C)2003,2004 Megan Potter
*/

/* Adapted from FoF/Tryptonite */

#include <kos.h>
#include <stdlib.h>
#include <plx/font.h>
#include <plx/prim.h>
#include <plx/context.h>

plx_font_t * font;
plx_fcxt_t * fcxt;

void pm_shutdown(void) {
    plx_fcxt_destroy(fcxt);
    plx_font_destroy(font);
}

void pm_init(void) {
    font = plx_font_load("/rd/font.txf");
    fcxt = plx_fcxt_create(font, PVR_LIST_TR_POLY);
    atexit(pm_shutdown);
}

void pm_drawbar(float pct, float posx, float posy, float posz,
                uint32_t c1, uint32_t c2) {
    const float len = 200.0f;

    plx_vert_inp(PLX_VERT, posx, posy + 6.0f, posz, c1);
    plx_vert_inp(PLX_VERT, posx, posy, posz, c1);
    plx_vert_inp(PLX_VERT, posx + len * pct, posy + 6.0f, posz, c1);
    plx_vert_inp(PLX_VERT_EOS, posx + len * pct, posy, posz, c1);

    plx_vert_inp(PLX_VERT, posx + len * pct, posy + 6.0f, posz, c2);
    plx_vert_inp(PLX_VERT, posx + len * pct, posy, posz, c2);
    plx_vert_inp(PLX_VERT, posx + len, posy + 6.0f, posz, c2);
    plx_vert_inp(PLX_VERT_EOS, posx + len, posy, posz, c2);
}

void pm_draw(void) {
    char str[128];
    float pct;
    float posx = 64, posy = 96, posz = 4500;

    // Get our stats
    pvr_stats_t stats;
    pvr_get_stats(&stats);

    // Make sure we're drawing into the right list.
    pvr_list_begin(PVR_LIST_TR_POLY);

    // Make a nice stat display
    sprintf(str, "fps: %.2f  buf: %lluns  reg: %lluns  rnd: %lluns",
            (double)stats.frame_rate,
            stats.buf_last_time,
            stats.reg_last_time,
            stats.rnd_last_time);
    plx_fcxt_setcolor4f(fcxt, 1, 1, 1, 1);
    plx_fcxt_setsize(fcxt, 16.0f);
    plx_fcxt_setpos(fcxt, posx, posy, posz);
    plx_fcxt_begin(fcxt);
    plx_fcxt_draw(fcxt, str);
    plx_fcxt_end(fcxt);

    sprintf(str, "avg fps: %.2f",
            (double)(60.0 * stats.frame_count / stats.vbl_count));
    plx_fcxt_setpos(fcxt, posx, posy + 16.0f, posz);
    plx_fcxt_begin(fcxt);
    plx_fcxt_draw(fcxt, str);
    plx_fcxt_end(fcxt);

    // Draw a bargraph for registration time
    plx_cxt_texture(NULL);
    plx_cxt_send(PVR_LIST_TR_POLY);

    posy += -40.0f;

    pct = ((float)stats.buf_last_time) * 60.0f / 1000000000.0f;
    pm_drawbar(pct, posx, posy, posz, 0xff00c0c0, 0xff007070);

    pct = ((float)stats.reg_last_time) * 60.0f / 1000000000.0f;
    posy += 8.0f;
    pm_drawbar(pct, posx, posy, posz, 0xff00c000, 0xff007000);

    // Draw a bargraph for rendering time
    pct = ((float)stats.rnd_last_time) * 60.0f / 1000000000.0f;
    posy += 8.0f;
    pm_drawbar(pct, posx, posy, posz, 0xffc0c000, 0xff707000);

    pvr_list_finish();
}
