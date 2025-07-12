/* KallistiOS ##version##

   lightgun.c
   Copyright (C) 2015 Lawrence Sebald
*/

#include <stdio.h>

#include <kos/dbgio.h>
#include <arch/timer.h>

#include <dc/biosfont.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/pvr.h>
#include <dc/video.h>

/* This little example isn't exactly anything fancy, but it does demonstrate the
   basics of getting the light gun up and running.

   Some things to note about using the light gun:
   1. Things work better if the player is aiming at a very bright section of the
      screen. That's why this example blanks the screen to white (only using
      black where it draws text). I'd suggest having at least one frame of a
      bright color before polling the gun, plus one frame while you're polling
      it. That way you take care of any interlacing effects.
   2. The light gun will (of course) only work with CRT televisions or monitors.
   3. You can only poll one gun at a time. This is a hardware limitation. You
      can switch back and forth between guns, but only one can actively be
      reading its position at a time.
   4. The light gun itself only returns data as if it were a normal controller.
      The trigger is mapped to the A button and you also have the D-Pad, B, and
      Start buttons, generally.
*/

int main(int argc, char *argv[]) {
    int x, y, gun = 0;
    maple_device_t *dev;
    cont_state_t *state;
    uint64_t last = 0, now;

    /* Do any printing to the screen and make it be black text on a white
       background (as much as we can anyway). I should eventually make it so you
       can specify this in fb_console... */
    dbgio_dev_select("fb");
    bfont_set_foreground_color(0x00000000);
    bfont_set_background_color(0xFFFFFFFF);
    pvr_init_defaults();

    /* Blank the whole screen to white. */
    for(y = 0; y < 480; ++y) {
        for(x = 0; x < 640; ++x) {
            vram_s[y * 640 + x] = 0xFFFF;
        }
    }

    for(;;) {
        /* Wait for vblank... */
        vid_waitvbl();
        now = timer_ms_gettime64();

        /* Blank the "play" area of the screen to white. */
        for(y = 0; y < 480; ++y) {
            for(x = 128; x < 640; ++x) {
                vram_s[y * 640 + x] = 0xFFFF;
            }
        }

        /* Did we hit the trigger last frame? If so, grab the counter values and
           print them out to the screen. */
        if(gun) {
            maple_gun_read_pos(&x, &y);
            printf("%d %d\n", x, y);
            gun = 0;
        }

        /* Grab the light gun and poll it for whether any interesting buttons
           are pressed. */
        if((dev = maple_enum_type(0, MAPLE_FUNC_LIGHTGUN))) {
            /* The light gun "status" is just that of its buttons. The data for
               positioning actually is read from a video register... */
            if((state = (cont_state_t *)maple_dev_status(dev))) {
                /* Exit if the user pressed start. */
                if((state->buttons & CONT_START))
                    break;

                /* The light gun's trigger is mapped to the A button. See if the
                   user is pulling the trigger and enable the gun if needed. */
                if((state->buttons & CONT_A) && last + 200 < now) {
                    maple_gun_enable(dev->port);
                    last = now;
                    gun = 1;
                }
            }
        }
    }

    return 0;
}
