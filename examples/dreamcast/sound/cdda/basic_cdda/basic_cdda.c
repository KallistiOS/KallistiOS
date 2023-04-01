/* KallistiOS ##version##

   basic_cdda.c
   (c)2002 Megan Potter
*/

#include <kos.h>

/*

This program demonstrates basic usage of the CDDA controls. It starts some
tracks playing in a loop and then lets you control it until you hit START.
Use the DPAD to control the left channel and A/B/X/Y to control the right
channel. You can use any attached controller.

Unfortunately this also demonstrates that the channel regs aren't set quite
correctly yet ^_^;

*/

int main(int argc, char **argv) {
    maple_device_t  *addr;
    cont_state_t    *st;
    int     i;
    int     done;
    int     lvol, rvol, lpan, rpan;

    /* Start the CDDA playing. Loop tracks 1-3, 4 times, and then stop */
    cdrom_cdda_play(1, 3, 4, CDDA_TRACKS);

    /* Wait for a controller button */
    done = 0;
    lvol = rvol = 15;
    lpan = 0;
    rpan = 31;

    while(!done) {
        i = 0;

        while((addr = maple_enum_type(i++, MAPLE_FUNC_CONTROLLER))) {
            /* Check for input */
            st = (cont_state_t *)maple_dev_status(addr);

            /* Exit */
            if(st->buttons & CONT_START) {
                done = 1;
                break;
            }

            /* Left channel */
            if(st->buttons & CONT_DPAD_UP)
                lvol++;

            if(st->buttons & CONT_DPAD_DOWN)
                lvol--;

            if(st->buttons & CONT_DPAD_LEFT)
                lpan--;

            if(st->buttons & CONT_DPAD_RIGHT)
                lpan++;

            /* Right channel */
            if(st->buttons & CONT_B)
                rvol++;

            if(st->buttons & CONT_A)
                rvol--;

            if(st->buttons & CONT_X)
                rpan--;

            if(st->buttons & CONT_Y)
                rpan++;

            /* Normalize */
            if(lvol < 0) lvol = 0;

            if(lvol > 15) lvol = 15;

            if(lpan < -16) lpan = -16;

            if(lpan > 31) lpan = 31;

            if(rvol < 0) rvol = 0;

            if(rvol > 15) rvol = 15;

            if(rpan < -16) rpan = -16;

            if(rpan > 31) rpan = 31;

            /* Set pan/vol */
            spu_cdda_volume(lvol, rvol);
            spu_cdda_pan(lpan, rpan);

            /* Sleep a little */
            usleep(100 * 1000);
        }
    }

    /* Stop the CDDA */
    cdrom_cdda_pause();

    /* Spin down the CD */
    cdrom_spin_down();

    return 0;
}


