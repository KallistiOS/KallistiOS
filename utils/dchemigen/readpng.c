/* KallistiOS ##version##

   readpng.c
   Copyright (C)2002 Jeffrey McBeth

*/

/* Modified for vqenc by Megan Potter */

#include <stdio.h>
#include <stdlib.h>

#include <png.h>        /* libpng header; includes zlib.h */
#include <zlib.h>
#include "readpng.h"    /* typedefs, common macros, public prototypes */

static png_structp png_ptr = NULL;
static png_infop info_ptr = NULL;

void readpng_version_info(void) {
    fprintf(stderr, "   Compiled with libpng %s; using libpng %s.\n",
            PNG_LIBPNG_VER_STRING, png_libpng_ver);
    fprintf(stderr, "   Compiled with zlib %s; using zlib %s.\n",
            ZLIB_VERSION, zlib_version);
}


/* return value = 0 for success, 1 for bad sig, 2 for bad IHDR, 3 for io, 4 for no mem */

uint32 readpng_init(FILE *infile) {
    uint8 sig[8];

    /* first do a quick check that the file really is a PNG image; could
     * have used slightly more general png_sig_cmp() function instead */

    if(fread(sig, 8, 1, infile) != 1) {
        return 3;
    }

    if(!png_check_sig(sig, 8))
        return 1;   /* bad signature */

    /* could pass pointers to user-defined error handlers instead of NULLs: */

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if(!png_ptr)
        return 4;   /* out of memory */

    info_ptr = png_create_info_struct(png_ptr);

    if(!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return 4;   /* out of memory */
    }

    png_init_io(png_ptr, infile);
    png_set_sig_bytes(png_ptr, 8);  /* we already read the 8 signature bytes */

    png_read_info(png_ptr, info_ptr);  /* read all PNG info up to image data */

    /* OK, that's all we need for now; return happy */

    return 0;
}

uint8 *readpng_get_image(uint32 *pChannels, uint32 *pRowbytes, int *pWidth, int *pHeight) {
    png_uint_32  width, height;
    int  bit_depth, color_type;
    uint8  *image_data = NULL;
    png_uint_32  i, rowbytes;
    png_bytepp  row_pointers = NULL;


    /* alternatively, could make separate calls to png_get_image_width(),
     * etc., but want bit_depth and color_type for later [don't care about
     * compression_type and filter_type => NULLs] */

    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
                 NULL, NULL, NULL);

    *pWidth = (int)width;
    *pHeight = (int)height;

    /* expand palette images to RGB, low-bit-depth grayscale images to 8 bits,
     * transparency chunks to full alpha channel; strip 16-bit-per-sample
     * images to 8 bits per sample; and convert grayscale to RGB[A] */

    if(color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_expand(png_ptr);

    if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand(png_ptr);

    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_expand(png_ptr);

    if(bit_depth == 16)
        png_set_strip_16(png_ptr);

    if(color_type == PNG_COLOR_TYPE_GRAY ||
            color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);

    /* all transformations have been registered; now update info_ptr data,
     * get rowbytes and channels, and allocate image memory */

    png_read_update_info(png_ptr, info_ptr);

    *pRowbytes = rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    *pChannels = (int)png_get_channels(png_ptr, info_ptr);

    if((image_data = (uint8 *)malloc(rowbytes * height)) == NULL) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    if((row_pointers = (png_bytepp)malloc(height * sizeof(png_bytep))) == NULL) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        free(image_data);
        image_data = NULL;
        return NULL;
    }

    Trace((stderr, "readpng_get_image:  channels = %d, rowbytes = %ld, height = %ld\n", *pChannels, rowbytes, height));


    /* set the individual row_pointers to point at the correct offsets */

    for(i = 0;  i < height;  ++i)
        row_pointers[i] = image_data + i * rowbytes;

    /* now we can go ahead and just read the whole image */

    png_read_image(png_ptr, row_pointers);

    free(row_pointers);
    row_pointers = NULL;

    png_read_end(png_ptr, NULL);

    return image_data;
}


void readpng_cleanup(void) {
    if(png_ptr && info_ptr) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        png_ptr = NULL;
        info_ptr = NULL;
    }
}
