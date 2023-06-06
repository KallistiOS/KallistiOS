/* GhettoPlay: an Ogg Vorbis browser and playback util
   (c)2000-2002 Megan Potter
   (c)2001 Thorsten Titze

   Distributed as an example for libdream 0.7
   Ported up to libdream 0.95
   Ported up to KOS 1.1.x
   Converted to OggVorbis

   Historical note: this is _really_ 2.0. There was an internally
   created Ghetto Play that I made to browse and test various S3Ms
   with early versions of the player. It _really_ deserved the name
   GHETTO Play =). This one, like Ghetto Pong, is a 2.0 that is more
   like "Pimpin' Play" ;-). However, sticking with tradition...

*/

#include <oggvorbis/sndoggvorbis.h>
#include <stdio.h>
#include "gp.h"

long bitrate;

int check_start() {
    MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)

    if(st->buttons & CONT_START) {
        printf("Pressed start\n");
        return 1;
    }

    MAPLE_FOREACH_END()

    return 0;
}

/* Render the mouse if they have one attached */
static int mx = 320, my = 240;
static int lmx[5] = {320, 320, 320, 320, 320},
                    lmy[5] = {240, 240, 240, 240, 240};
void mouse_render() {
    int i;
    int atall = 0;

    MAPLE_FOREACH_BEGIN(MAPLE_FUNC_MOUSE, mouse_state_t, st)
    atall = 1;

    if(st->dx || st->dy) {

        mx += st->dx;
        my += st->dy;

        if(mx < 0) mx = 0;

        if(mx > 640) mx = 640;

        if(my < 0) my = 0;

        if(my > 480) my = 480;

        lmx[0] = mx;
        lmy[0] = my;
    }

    MAPLE_FOREACH_END()

    if(atall) {
        for(i = 4; i > 0; i--) {
            lmx[i] = lmx[i - 1];
            lmy[i] = lmy[i - 1];
        }

        draw_poly_mouse(mx, my, 1.0f);

        for(i = 1; i < 5; i++)
            draw_poly_mouse(lmx[i], lmy[i], 0.8f * (5 - i) / 6.0f);
    }
}

/* Update the VMU LCD */
#include "vmu_ghetto.h"
#include "vmu_play.h"
#include "vmu_ghettoplay.h"
static int cycle = 0, phase = 1;
void vmu_lcd_update() {
    switch(cycle) {
        case 0: {   /* Getto/Play/GettoPlay */
            if(phase == 1) {
                vmu_set_icon(vmu_ghetto_xpm);
            }
            else if(phase == 60) {
                vmu_set_icon(vmu_play_xpm);
            }
            else if(phase == 120) {
                vmu_set_icon(vmu_ghettoplay_xpm);
            }
            else if(phase == 180) {
                phase = 0;
                cycle = 1;
            }
        }
        break;
        case 1: {   /* Flashing */
            if((phase % 20) == 0) {
                vmu_set_icon(vmu_ghettoplay_xpm);
            }
            else if((phase % 20) == 10) {
                vmu_set_icon(NULL);
            }

            if(phase == 60) {
                cycle = 0;
                phase = 0;
            }
        }
        break;
    }

    phase++;
}

extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);

/* Program entry */
int main(int argc, char **argv) {
    /* Do basic setup */
    pvr_init_defaults();

    /* Initialize oggvorbis player thread */
    snd_stream_init();
    sndoggvorbis_init();

    /* Setup the mouse/font texture */
    setup_util_texture();

    /* Setup background display */
    bkg_setup();

    while(!check_start()) {
        pvr_wait_ready();
        pvr_scene_begin();
        pvr_list_begin(PVR_LIST_OP_POLY);

        /* Opaque list *************************************/
        bkg_render();

        /* End of opaque list */
        pvr_list_finish();
        pvr_list_begin(PVR_LIST_TR_POLY);

        /* Translucent list ********************************/

        /* Top Banner */
        draw_poly_box(0.0f, 10.0f, 640.0f, 20.0f + (24.0f * 2.0f) + 10.0f, 90.0f,
                      0.3f, 0.2f, 0.5f, 0.0f, 0.5f, 0.1f, 0.8f, 0.2f);
        draw_poly_strf(5.0f, 20.0f, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                       "     GhettoPlay  (C)2000-2002 by Megan Potter     ");
        draw_poly_strf(5.0f, 48.0f, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                       "      sndoggvorbis (C)2001 by Thorsten Titze      ");

        /* Song menu */
        song_menu_render();

        /* File Information */
        draw_poly_box(20.0f, 440.0f - 96.0f + 4, 640.0f - 20.0f, 440.0f, 90.0f,
                      0.3f, 0.2f, 0.5f, 0.0f, 0.5f, 0.1f, 0.8f, 0.2f);

        draw_poly_strf(30.0f, 440.0f - 96.0f + 6, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f, "File Information:");
        draw_poly_strf(640.0f - 180.0f - 12.0f + (6 * 12.0f), 440.0f - 96.0f + 6, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f, "bit/s");
        draw_poly_strf(30.0f, 440.0f - 96.0f + 6 + 24.0f + 10.0f, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f, "Author:");
        draw_poly_strf(30.0f, 440.0f - 96.0f + 6 + 48.0f + 10.0f, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f, "Title:");
        draw_poly_strf(320.0f, 440.0f - 96.0f + 6 + 24.0f + 10.0f, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f, "Genre:");

        /* If we're playing a file fill out the File information */
        if(sndoggvorbis_isplaying()) {
            char bitrate[7];
            long bitrateval = sndoggvorbis_getbitrate();
            snprintf(bitrate, sizeof(bitrate), "%6ld", bitrateval);
            char * artist, * title, * genre;

            artist = sndoggvorbis_getartist();

            if(!artist) artist = "[none]";

            title = sndoggvorbis_gettitle();

            if(!title) title = "[none]";

            genre = sndoggvorbis_getgenre();

            if(!genre) genre = "[none]";

            draw_poly_strf(640.0f - 180.0f - 12.0f - 12.0f, 440.0f - 96.0f + 6, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f, bitrate);
            draw_poly_strf(30.0f + (12.0f * 7), 440.0f - 96.0f + 6 + 24.0f + 10.0f, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f, artist);
            draw_poly_strf(30.0f + (12.0f * 6), 440.0f - 96.0f + 6 + 48.0f + 10.0f, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f, title);
            draw_poly_strf(320.0f + (12.0f * 6), 440.0f - 96.0f + 6 + 24.0f + 10.0f, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f, genre);
        }

        /* Render the mouse if they move it.. it doesn't do anything
           but it's cool looking ^_^ */
        mouse_render();

        /* End of translucent list */
        pvr_list_finish();

        /* Finish the frame *******************************/
        pvr_scene_finish();

        /* Update the VMU LCD */
        vmu_lcd_update();
    }

    sndoggvorbis_shutdown();

    /* Stop the sound */
    spu_disable();

    return 0;
}





