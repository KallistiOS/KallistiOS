/* KallistiOS ##version##

   speedtest.c
   Copyright (C) 2003, 2024 Megan Potter
   Copyright (C) 2024 Andress Barajas
*/

/*
   This file sets up the Dreamcast network speed test server, which listens 
   for HTTP requests on the network and performs download and upload speed tests. 
   It creates a thread to handle incoming client connections and displays a basic 
   status message on the Dreamcast screen. The browser-based front-end (index.html)
   is used to run the test and display the results. The user can press the "START" 
   button on the Dreamcast controller to shut down the server and exit the 
   application.
*/

#include <kos/init.h>
#include <kos/dbgio.h>

#include <dc/video.h>
#include <dc/biosfont.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

#include "speedtest.h"

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_NET);

int main(int argc, char **argv) {
    vid_clear(23,86,155);
    bfont_draw_str(vram_s + 20 * 640 + 20, 640, 0, "SpeedTest Server active");
    bfont_draw_str(vram_s + 44 * 640 + 20, 640, 0, "Press START to shutdown.");

    thd_create(DETACHED_THREAD, server_thread, NULL);

    while(true) {
        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)

        if(st->buttons & CONT_START)
            return 0;

        MAPLE_FOREACH_END()
    }

    return 0;
}
