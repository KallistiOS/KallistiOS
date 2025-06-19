/* KallistiOS ##version##

   screenshot.c
   Copyright (C) 2024 Andy Barajas

*/

/* 
   This program demonstrates how to use the vid_screen_shot() function
   to capture and save a screenshot in the PPM format to your computer
   using the DC Tool. This tool requires the '-c "."' command-line argument
   to operate correctly.

   The program cycles through a color gradient background and allows user
   interaction to capture screenshots or exit the program.

   Usage:
   Ensure the '/pc/' directory path is correctly specified in the vid_screen_shot()
   function call so that the screenshot.ppm file is saved in the appropriate
   directory on your computer.
*/

#include <stdio.h>

#include <dc/video.h>
#include <dc/fmath.h>
#include <dc/maple.h>
#include <dc/biosfont.h>
#include <dc/maple/controller.h>

#include <kos/thread.h>

#define SHOW_BLACK_BG  true

/* Keeps track of the amount of screenshots you have taken */
static int counter = 0;

int main(int argc, char **argv) {
    uint8_t r, g, b;
    uint32_t t = 0;
    char filename[256];

    /* Adjust frequency for faster or slower transitions */
    float frequency = 0.01; 
    
    maple_device_t *cont;
    cont_state_t *state;

    /* Set the video mode */
    vid_set_mode(DM_640x480, PM_RGB565);

    while(1) {
        if((cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER)) != NULL) {
            state = (cont_state_t *)maple_dev_status(cont);

            if(state == NULL)
                break;

            if(state->buttons & CONT_START)
                break;

            if(state->buttons & CONT_A) {
                sprintf(filename, "/pc/screenshot%03d.ppm", counter);
                vid_screen_shot(filename);
                counter = (counter + 1) % 1000;
            }
        }

        /* Wait for VBlank */
        vid_waitvbl();
        
        /* Calculate next background color */
        r = (uint8_t)((fsin(frequency * t + 0) * 127.5) + 127.5);
        g = (uint8_t)((fsin(frequency * t + 2 * F_PI / 3) * 127.5) + 127.5);
        b = (uint8_t)((fsin(frequency * t + 4 * F_PI / 3) * 127.5) + 127.5);

        /* Increment t to change color in the next cycle */
        t = (t + 1) % INT32_MAX;

        /* Draw Background */
        vid_clear(r, g, b);

        /* Draw Foreground */
        bfont_draw_str_vram_fmt(24, 336, SHOW_BLACK_BG, 
            "Press Start to exit\n\nPress A to take a screen shot");

        vid_flip(-1);
    }

    return 0;
}
