/* KallistiOS ##version##

   font.c
   (c)2002 Megan Potter

   Note: axaxax.txf is Ray Larabie's AXAXAX font from his site:
   http://www.larabiefonts.com/
   It was converted using gentexfnt.
*/

#include <kos.h>
#include <plx/font.h>
#include <plx/sprite.h>
#include <plx/list.h>
#include <plx/dr.h>

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

int main(int argc, char **argv) {
    plx_font_t * fnt;
    plx_fcxt_t * cxt;
    int done = 0;
    point_t w;

    cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y, (cont_btn_callback_t) arch_exit);

    pvr_init_defaults();

    fnt = plx_font_load("/rd/axaxax.txf");
    cxt = plx_fcxt_create(fnt, PVR_LIST_TR_POLY);

    w.x = 30.0f;
    w.y = 50.0f;
    w.z = 10.0f;

    while(!done) {
        pvr_wait_ready();
        pvr_scene_begin();
        pvr_list_begin(PVR_LIST_OP_POLY);
        pvr_list_begin(PVR_LIST_TR_POLY);

        plx_fcxt_begin(cxt);
        plx_fcxt_setpos_pnt(cxt, &w);
        plx_fcxt_draw(cxt, "This is a test!");
        plx_spr_inp(256, 256, 320, 240, 20, 0xffffffff);    // texture test
        plx_fcxt_end(cxt);

        pvr_scene_finish();

        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, t)

        if(t->buttons & CONT_START)
            done = 1;

        MAPLE_FOREACH_END()
    }

    if(cxt)
        plx_fcxt_destroy(cxt);

    if(fnt)
        plx_font_destroy(fnt);

    return 0;
}


