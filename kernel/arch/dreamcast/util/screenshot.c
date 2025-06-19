/* KallistiOS ##version##

   screenshot.c

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2008 Donald Haase
   Copyright (C) 2024 Andy Barajas

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dc/video.h>
#include <kos/dbglog.h>
#include <kos/fs.h>
#include <arch/irq.h>

#include <arch/timer.h>

/*
    Provides a very simple screen shot facility (dumps raw 24bpp RGB image 
    data from the currently viewed framebuffer).

    This will now work with any of the supported video modes.
*/

#define BYTES_PER_PIXEL 3

size_t vid_screen_shot_data(uint8_t **buffer) {
    int i, numpix;
    uint32_t pixel, pixel1, pixel2;
    size_t buffer_size;
    uint8_t *data_ptr;
    uint32_t save;

    /* Allocate buffer to store the 24bpp image data */
    numpix = vid_mode->width * vid_mode->height;
    buffer_size = numpix * BYTES_PER_PIXEL;
    if(!buffer_size) {
        dbglog(DBG_ERROR, "vid_screen_shot_data: invalid video mode.\n");
        return 0;
    }

    *buffer = (uint8_t *)malloc(buffer_size);
    if(*buffer == NULL) {
        dbglog(DBG_ERROR, "vid_screen_shot_data: can't allocate memory\n"); 
        return 0;
    }

    data_ptr = *buffer;

    /* Disable interrupts */
    save = irq_disable();

    /* Write out each pixel as 24-bits */
    switch(vid_mode->pm) {
        case PM_RGB555:  /* (15-bit) */
            /* Process two 16-bit pixels at a time */
            for(i = 0; i < numpix / 2; i++) {
                pixel = vram_l[i];
                pixel1 = pixel & 0xFFFF;
                pixel2 = pixel >> 16;

                /* Process the first pixel */
                data_ptr[i * 6 + 0] = (((pixel1 >> 10) & 0x1f) << 3); /* R */
                data_ptr[i * 6 + 1] = (((pixel1 >> 5) & 0x1f) << 3);  /* G */
                data_ptr[i * 6 + 2] = (((pixel1 >> 0) & 0x1f) << 3);  /* B */

                /* Process the second pixel */
                data_ptr[i * 6 + 3] = (((pixel2 >> 10) & 0x1f) << 3); /* R */
                data_ptr[i * 6 + 4] = (((pixel2 >> 5) & 0x1f) << 3);  /* G */
                data_ptr[i * 6 + 5] = (((pixel2 >> 0) & 0x1f) << 3);  /* B */
            }
            break;
        case PM_RGB565: /* (16-bit) */
            /* Process two 16-bit pixels at a time */
            for(i = 0; i < numpix / 2; i++) {
                pixel = vram_l[i];
                pixel1 = pixel & 0xFFFF;
                pixel2 = pixel >> 16;

                /* Process the first pixel */
                data_ptr[i * 6 + 0] = (((pixel1 >> 11) & 0x1f) << 3); /* R */
                data_ptr[i * 6 + 1] = (((pixel1 >> 5) & 0x3f) << 2);  /* G */
                data_ptr[i * 6 + 2] = (((pixel1 >> 0) & 0x1f) << 3);  /* B */

                /* Process the second pixel */
                data_ptr[i * 6 + 3] = (((pixel2 >> 11) & 0x1f) << 3); /* R */
                data_ptr[i * 6 + 4] = (((pixel2 >> 5) & 0x3f) << 2);  /* G */
                data_ptr[i * 6 + 5] = (((pixel2 >> 0) & 0x1f) << 3);  /* B */
            }
            break;
        case PM_RGB888P:  /* (24-bit) */
            for(i = 0; i < numpix; i++) {
                data_ptr[i * 3 + 0] = ((uint8_t *)vram_l)[i * 3 + 2]; /* R */
                data_ptr[i * 3 + 1] = ((uint8_t *)vram_l)[i * 3 + 1]; /* G */
                data_ptr[i * 3 + 2] = ((uint8_t *)vram_l)[i * 3 + 0]; /* B */
            }
            break;
        case PM_RGB0888:  /* (32-bit) */
            for(i = 0; i < numpix; i++) {
                pixel = vram_l[i];
                data_ptr[i * 3 + 0] = (((pixel >> 16) & 0xff)); /* R */
                data_ptr[i * 3 + 1] = (((pixel >> 8) & 0xff));  /* G */
                data_ptr[i * 3 + 2] = (((pixel >> 0) & 0xff));  /* B */
            }
            break;
        default:
            dbglog(DBG_ERROR, "vid_screen_shot_data: can't process pixel mode %d\n", vid_mode->pm); 
            free(*buffer);
            *buffer = NULL;
            irq_restore(save);
            return 0;
    }

    /* Restore interrupts */
    irq_restore(save);

    return buffer_size;
}

/* Destination file system must be writeable and have enough free space. */
int vid_screen_shot(const char *destfn) {
    file_t   f;
    uint8_t *buffer;
    size_t buffer_size;

    char *header;
    size_t header_size;

    /* Open output file */
    f = fs_open(destfn, O_WRONLY | O_TRUNC);
    if(!f) {
        dbglog(DBG_ERROR, "vid_screen_shot: can't open output file '%s'\n", destfn);
        return -1;
    }

    /* Measure PPM header length */
    header_size = snprintf(NULL, 0, "P6\n#KallistiOS Screen Shot\n%d %d\n255\n", vid_mode->width, vid_mode->height) + 1;

    /* Allocate header buffer on the stack */
    header = (char *)alloca(header_size);

    /* Create PPM header */
    sprintf(header, "P6\n#KallistiOS Screen Shot\n%d %d\n255\n", vid_mode->width, vid_mode->height);
    header_size = strlen(header);

    /* Generate image data */
    buffer_size = vid_screen_shot_data(&buffer);
    if(!buffer_size) {
        dbglog(DBG_ERROR, "vid_screen_shot: couldn't generate image data\n");
        return -1;
    }

    /* Write the data */
    if(fs_write(f, header, header_size) != (ssize_t)(header_size) ||
       fs_write(f, buffer, buffer_size) != (ssize_t)(buffer_size)) {
        dbglog(DBG_ERROR, "vid_screen_shot: can't write data to output file '%s'\n", destfn);
        fs_close(f);
        free(buffer);
        return -1;
    }

    fs_close(f);
    free(buffer);

    dbglog(DBG_INFO, "vid_screen_shot: written to output file '%s'\n", destfn);

    return 0;
}
