/* KallistiOS ##version##

   font.cpp
   (c)2002 Megan Potter

   Note: axaxax.txf is Ray Larabie's AXAXAX font from his site:
   http://www.larabiefonts.com/
   It was converted using gentexfnt.
*/

#include <kos.h>
#include <tsu/font.h>
#include <tsu/texture.h>
#include <plx/sprite.h>

/*
  Shows off some very basic usage of the font and texture objects
  with Parallax as the vertex system rather than the Tsunami scene
  graphs.
 */

extern uint8 romdisk[];
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);

int main(int argc, char **argv) {
    int done = 0;

    cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
                      (cont_btn_callback_t)arch_exit);

    pvr_init_defaults();

    RefPtr<Font> fnt = new Font("/rd/axaxax.txf");
    fnt->setSize(24.0f);

    RefPtr<Texture> txr = new Texture("/rd/logo.png", true);

    pvr_set_bg_color(0.2f, 0.0f, 0.4f);

    while(!done) {
        pvr_wait_ready();
        pvr_scene_begin();
        pvr_list_begin(PVR_LIST_OP_POLY);
        pvr_list_begin(PVR_LIST_TR_POLY);

        fnt->draw(30.0f, 50.0f, 10.0f, "This is a test!");
        txr->sendHdr(PVR_LIST_TR_POLY);
        plx_spr_inp(txr->getW(), txr->getH(), 320, 240, 20, 0xffffffff);

        pvr_scene_finish();

        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, t)

        if(t->buttons & CONT_START)
            done = 1;

        MAPLE_FOREACH_END()
    }

    return 0;
}


