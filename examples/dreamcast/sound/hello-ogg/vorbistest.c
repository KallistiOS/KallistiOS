/* KallistiOS Ogg/Vorbis Decoder Library
 *
 * vorbistest.c
 * (c)2001 Thorsten Titze
 *
 * This small program demonstrates the usage of the liboggvorbisplay library
 * to playback audio data encoded with the Vorbis codec.
 *
 * NOTE:
 * You have to put an Ogg File into the RAM Disk which should be called
 * "test.ogg". Or you change the filename in this sourcecode
 */

#include <kos.h>
#include <oggvorbis/sndoggvorbis.h>
#include "display.c"

long bitrateold, bitratenew;

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

int main(int argc, char **argv) {
    maple_device_t *cont;
    cont_state_t *state;

    print_d("Vorbis Decoder Library Example Program\n\n");

    snd_stream_init();
    sndoggvorbis_init();
    print_d("sndoggvorbis_init();  called...\n");

    sndoggvorbis_start("/rd/test.ogg", 0);
    print_d("sndoggvorbis_start(\"/rd/test.ogg\",0);  called...\n\n");

    printf("main: Vorbisfile artist=%s\r\n", sndoggvorbis_getartist());
    printf("main: Vorbisfile title=%s\r\n", sndoggvorbis_gettitle());


    print_d("The OggVorbis File now plays within a thread !\n\n");
    print_d("Press START to exit and (Y) to re-start playing...");

    while(1) {
        cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

        if(cont) {
            state = (cont_state_t *)maple_dev_status(cont);

            if(!state)
                break;

            if(state->buttons & CONT_START)
                break;

            if(state->buttons & CONT_Y) {
                printf("main: restarting oggvorbis\r\n");
                sndoggvorbis_stop();
                sndoggvorbis_start("/rd/test.ogg", 0);
            }

            // timer_spin_sleep(10);    <-- causes infinite loop sometimes!
            thd_sleep(10);
            bitratenew = sndoggvorbis_getbitrate();

            if(bitratenew != bitrateold) {
                printf("main: Vorbisfile current bitrate %ld\r\n", bitratenew);
                bitrateold = bitratenew;
            }
        }

    }

    sndoggvorbis_stop();
    sndoggvorbis_shutdown();
    snd_stream_shutdown();
    return 0;
}

