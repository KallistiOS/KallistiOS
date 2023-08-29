/* Opusplay for KallistiOS ##version##

   opustest.c
   Simple "Hello World" style example of Opus playback.

   Copyright (C) 2015 Lawrence Sebald
*/

#include <stdio.h>

#include <kos/dbgio.h>

#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/sound/stream.h>

#include <kos/init.h>
#include <kos/thread.h>

#include <opusplay/opusplay.h>

int main(int argc, char **argv) {
    maple_device_t *cont;
    cont_state_t *state;

    dbgio_dev_select("fb");

    printf("Opus Decoder Library Example Program\n\n");

    opusplay_init();

    if(opusplay_play_file("/rd/test.opus", 0)) {
        printf("Cannot play /rd/test.opus!\n");
        printf("Did you remember to put an opus file in the\n"
               "romdisk before compiling?\n");
        thd_sleep(10 * 1000);

        opusplay_shutdown();
        snd_stream_shutdown();
        return 0;
    }

    printf("The Opus file should now be playing in a thread...\n\n");
    printf("Press START to exit and (Y) to restart playback.\n");

    while(1) {
        if((cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER))) {
            if((state = (cont_state_t *)maple_dev_status(cont))) {
                if(state->buttons & CONT_START)
                    break;

                if(state->buttons & CONT_Y) {
                    opusplay_stop();
                    opusplay_play_file("/rd/test.opus", 0);
                }
            }

            thd_sleep(100);
        }

    }

    printf("Cleaning up...\n");
    opusplay_stop();
    opusplay_shutdown();
    snd_stream_shutdown();
    return 0;
}
