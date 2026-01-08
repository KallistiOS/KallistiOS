/* KallistiOS ##version##

   util/fb_console.c
   Copyright (C) 2009 Lawrence Sebald
   Copyright (C) 2026 Donald Haase

*/

#include <string.h>
#include <errno.h>
#include <kos/dbgio.h>
#include <kos/platform.h>
#include <dc/fb_console.h>
#include <dc/video.h>
#include <dc/biosfont.h>
#include <dc/minifont.h>

/* This is a very simple dbgio interface for doing debug to the framebuffer with
   the biosfont or minifont functionality. Basically, this was written to aid in debugging
   the network stack, and I figured other people would probably get some use out
   of it as well. */

static uint16_t *fb;
static size_t fb_w, fb_h;
static size_t cur_x, cur_y;
static size_t min_x, min_y, max_x, max_y;

static size_t FONT_CHAR_WIDTH;
static size_t FONT_CHAR_HEIGHT;

static int fb_init(void) {
    if(KOS_PLATFORM_IS_NAOMI) {
        FONT_CHAR_WIDTH = 8;
        FONT_CHAR_HEIGHT = 16;
    }
    else {
        bfont_set_encoding(BFONT_CODE_ISO8859_1);
        FONT_CHAR_WIDTH = BFONT_THIN_WIDTH;
        FONT_CHAR_HEIGHT = BFONT_HEIGHT;
    }

    /* Init based on current video mode, defaulting to 640x480x16bpp. */
    if(vid_mode == 0)
        dbgio_fb_set_target(NULL, 640, 480, 32, 32);
    else
        dbgio_fb_set_target(NULL, vid_mode->width, vid_mode->height, 32, 32);

    return 0;
}

static int fb_write(int c) {
    uint16_t *t = fb;

    if(!t)
        t = vram_s;

    if(c != '\n') {
        if(KOS_PLATFORM_IS_NAOMI)
            minifont_draw(t + cur_y * fb_w + cur_x, fb_w, c);
        else
            bfont_draw(t + cur_y * fb_w + cur_x, fb_w, 1, c);
        cur_x += FONT_CHAR_WIDTH;
    }

    /* If we have a newline or we've gone past the end of the line, advance down
       one line. */
    if(c == '\n' || cur_x + FONT_CHAR_WIDTH > max_x) {
        cur_y += FONT_CHAR_HEIGHT;
        cur_x = min_x;

        /* If going down a line put us over the edge of the screen, move
           everything up a line, fixing the problem. */
        if(cur_y + FONT_CHAR_HEIGHT > max_y) {
            memcpy(t + min_y * fb_w, t + (min_y + FONT_CHAR_HEIGHT) * fb_w,
                    (cur_y - min_y - FONT_CHAR_HEIGHT) * fb_w * 2);
            cur_y -= FONT_CHAR_HEIGHT;
            memset(t + cur_y * fb_w, 0, FONT_CHAR_HEIGHT * fb_w * 2);
        }
    }

    return 1;
}

static int fb_write_buffer(const uint8_t *data, int len, int xlat) {
    int rv = len;

    (void)xlat;

    while(len--) {
        fb_write((int)(*data++));
    }

    return rv;
}

dbgio_handler_t dbgio_fb = {
    .name = "fb",
    .init = fb_init,
    .write = fb_write,
    .write_buffer = fb_write_buffer
};

void dbgio_fb_set_target(uint16_t *t, int w, int h, int borderx, int bordery) {
    /* Set up all the new parameters. */
    fb = t;

    fb_w = w;
    fb_h = h;
    min_x = borderx;
    min_y = bordery;
    max_x = fb_w - borderx;
    max_y = fb_h - bordery;
    cur_x = min_x;
    cur_y = min_y;
}
