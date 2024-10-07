/* KallistiOS ##version##

   kos_img.c
   (c)2002 Jeffrey McBeth, Megan Potter

   Based on Jeff's png_load_texture routine, but loads into a
   KOS plat-independent image.
*/

#include <assert.h>
#include <png.h>
#include <stdlib.h>
#include "readpng.h"
#include "get_image.h"

/* load an n x n texture from a png */

/* not to be used outside of here */
void _png_copy_texture(uint8 *buffer, uint8 *temp_tex,
                       uint32 channels, uint32 stride,
                       uint32 w, uint32 h) {
    uint32 i, j;
    uint8 *ourbuffer;
    uint8 *pRow;

    for(i = 0; i < h; i++) {
        pRow = &buffer[i * stride];
        ourbuffer = &temp_tex[i * w * 4];

        if(channels == 3) {
            for(j = 0; j < w; j++) {
                ourbuffer[j * 4 + 0] = 0xff;
                ourbuffer[j * 4 + 1] = pRow[j * 3];
                ourbuffer[j * 4 + 2] = pRow[j * 3 + 1];
                ourbuffer[j * 4 + 3] = pRow[j * 3 + 2];
            }
        }
        else if(channels == 4) {
            for(j = 0; j < w; j++) {
                ourbuffer[j * 4 + 0] = pRow[j * 4 + 3];
                ourbuffer[j * 4 + 1] = pRow[j * 4 + 0];
                ourbuffer[j * 4 + 2] = pRow[j * 4 + 1];
                ourbuffer[j * 4 + 3] = pRow[j * 4 + 2];
            }
        }
    }
}

int get_image_png(const char * filename, image_t * image) {
    uint8 *temp_tex;

    /* More stuff */
    uint8   *buffer;    /* Output row buffer */
    uint32  row_stride; /* physical row width in output buffer */
    uint32  channels;   /* 3 for RGB 4 for RGBA */

    FILE    *infile;    /* source file */

    assert(image != NULL);

    if((infile = fopen(filename, "rb")) == 0) {
        printf("png_to_texture: can't open %s\n", filename);
        return -1;
    }

    /* Step 1: Initialize loader */
    if(readpng_init(infile)) {
        fclose(infile);
        return -2;
    }

    /* Step 1.5: Create output kos_img_t */
    /* rv = (kos_img_t *)malloc(sizeof(kos_img_t)); */

    /* Step 2: Read file */
    buffer = readpng_get_image(&channels, &row_stride, &image->w, &image->h);
    temp_tex = (uint8 *)malloc(sizeof(uint8) * 4 * image->w * image->h);
    image->data = (unsigned char *)temp_tex;
    image->bpp = 4;
    image->stride = image->w * 4;

    _png_copy_texture(buffer, temp_tex,
                      channels, row_stride,
                      image->w, image->h);

    /* Step 3: Finish decompression */
    free(buffer);
    readpng_cleanup();

    fclose(infile);

    /* And we're done! */
    return 0;
}
