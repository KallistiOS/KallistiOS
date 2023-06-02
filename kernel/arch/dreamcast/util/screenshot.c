/* KallistiOS ##version##

   screenshot.c
   (c)2002 Megan Potter
   (c)2008 Donald Haase
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dc/video.h>
#include <kos/fs.h>
#include <arch/irq.h>

/*

Provides a very simple screen shot facility (dumps raw RGB PPM files from the
currently viewed framebuffer).

Destination file system must be writeable and have enough free space.

This will now work with any of the supported video modes.

*/

int vid_screen_shot(const char *destfn) {
    file_t  f;
    int i, numpix;
    uint8   *buffer;
    char    header[256];
    uint32  save;
    uint32  pixel;  /* to fit 888 mode */
    uint8   r, g, b;
    uint8   bpp;

    bpp = 3;    /* output to ppm is 3 bytes per pixel */
    numpix = vid_mode->width * vid_mode->height;

    /* Allocate a new buffer so we can blast it all at once */
    buffer = (uint8 *)malloc(numpix * bpp);

    if(buffer == NULL) {
        dbglog(DBG_ERROR, "vid_screen_shot: can't allocate ss memory\n");
        return -1;
    }

    /* Disable interrupts */
    save = irq_disable();

    /* Write out each pixel as 24 bits */
    switch(vid_mode->pm) {
        case(PM_RGB555): {
            for(i = 0; i < numpix; i++) {
                pixel = vram_s[i];
                r = (((pixel >> 10) & 0x1f) << 3);
                g = (((pixel >>  5) & 0x1f) << 3);
                b = (((pixel >>  0) & 0x1f) << 3);
                buffer[i * 3 + 0] = r;
                buffer[i * 3 + 1] = g;
                buffer[i * 3 + 2] = b;
            }

            break;
        }
        case(PM_RGB565): {
            for(i = 0; i < numpix; i++) {
                pixel = vram_s[i];
                r = (((pixel >> 11) & 0x1f) << 3);
                g = (((pixel >>  5) & 0x3f) << 2);
                b = (((pixel >>  0) & 0x1f) << 3);
                buffer[i * 3 + 0] = r;
                buffer[i * 3 + 1] = g;
                buffer[i * 3 + 2] = b;
            }

            break;
        }
        case(PM_RGB888): {
            for(i = 0; i < numpix; i++) {
                pixel = vram_l[i];
                r = (((pixel >> 16) & 0xff));
                g = (((pixel >>  8) & 0xff));
                b = (((pixel >>  0) & 0xff));
                buffer[i * 3 + 0] = r;
                buffer[i * 3 + 1] = g;
                buffer[i * 3 + 2] = b;
            }

            break;
        }
        default: {
            dbglog(DBG_ERROR, "vid_screen_shot: can't process pixel mode %d\n", vid_mode->pm);
            irq_restore(save);
            free(buffer);
            return -1;
        }
    }

    irq_restore(save);

    /* Open output file */
    f = fs_open(destfn, O_WRONLY | O_TRUNC);

    if(!f) {
        dbglog(DBG_ERROR, "vid_screen_shot: can't open output file '%s'\n", destfn);
        free(buffer);
        return -1;
    }

    /* Write a small header */
    sprintf(header, "P6\n#KallistiOS Screen Shot\n%d %d\n255\n", vid_mode->width, vid_mode->height);

    if(fs_write(f, header, strlen(header)) != (ssize_t)strlen(header)) {
        dbglog(DBG_ERROR, "vid_screen_shot: can't write header to output file '%s'\n", destfn);
        fs_close(f);
        free(buffer);
        return -1;
    }

    /* Write the data */
    if(fs_write(f, buffer, numpix * bpp) != (ssize_t)(numpix * bpp)) {
        dbglog(DBG_ERROR, "vid_screen_shot: can't write data to output file '%s'\n", destfn);
        fs_close(f);
        free(buffer);
        return -1;
    }

    fs_close(f);
    free(buffer);

    return 0;
}
