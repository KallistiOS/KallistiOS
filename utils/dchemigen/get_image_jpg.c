
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <jpeglib.h>
#include "get_image.h"

/* get_image() is merely a copy of Andrew's jpeg_to_texture routine */
int get_image_jpg(const char *filename, image_t *image)
{	
	FILE *infile;
	int i;
	
	/* This struct contains the JPEG decompression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 */
	struct jpeg_decompress_struct cinfo;
    
	struct jpeg_error_mgr jerr;
    
	/* More stuff */
	JSAMPARRAY buffer;	/* Output row buffer */

	/* In this example we want to open the input file before doing anything else,
	 * so that the setjmp() error recovery below can assume the file is open.
	 * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	 * requires it in order to read binary files.
	 */
	 
	infile = fopen(filename, "rb");
	if (infile == NULL) {
		return -errno;
	}
        
	/* Step 1: allocate and initialize JPEG decompression object */
    
	/* We set up the normal JPEG error routines */
	cinfo.err = jpeg_std_error(&jerr);
    
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);
    
	/* Step 2: specify data source (eg, a file) */
	jpeg_stdio_src(&cinfo, (FILE*)infile);
    
	/* Step 3: read file parameters with jpeg_read_header() */
	(void)jpeg_read_header(&cinfo, TRUE);
	/* We can ignore the return value from jpeg_read_header since
	 *   (a) suspension is not possible with the stdio data source, and
	 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	 * See libjpeg.doc for more info.
	 */
	 
	(void)jpeg_start_decompress(&cinfo);

	/* Allocate the output buffers */
	image->w = cinfo.image_width;
	image->h = cinfo.image_height;
	image->bpp = cinfo.output_components;
	image->stride = 4*image->w;
	image->data = (unsigned char*)malloc(image->stride * image->h);
	if (image->data == NULL) {
		/* ouch */
		fclose(infile);
		return -ENOMEM;
	}
    
	/* Step 4: set parameters for decompression */
	cinfo.scale_denom = 1; /* must be 1, 2, 4, or 8 */

	/* We may need to do some setup of our own at this point before reading
	 * the data.  After jpeg_start_decompress() we have the correct scaled
	 * output image dimensions available, as well as the output colormap
	 * if we asked for color quantization.
	 * In this example, we need to make an output work buffer of the right size.
	 */ 
	/* JSAMPLEs per row in output buffer */

	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, image->stride, 1);
    
	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */
    
	/* Here we use the library's state variable cinfo.output_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 */
	while (cinfo.output_scanline < cinfo.output_height) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
	 	* Here the array is only one element long, but you could ask for
	 	* more than one scanline at a time if that's more convenient.
	 	*/
		(void)jpeg_read_scanlines(&cinfo, buffer, 1);

		for (i=0; i<image->w; i++) {
			int offs = (cinfo.output_scanline-1) * 4*image->w;
			image->data[offs + i*4 + 0] = 0xff;
			image->data[offs + i*4 + 1] = buffer[0][i*3 + 0];
			image->data[offs + i*4 + 2] = buffer[0][i*3 + 1];
			image->data[offs + i*4 + 3] = buffer[0][i*3 + 2];
		}
		/* memcpy(image->data + (cinfo.output_scanline-1) * image->stride, buffer[0], image->stride); */
	}
	
	/* Step 7: Finish decompression */
	(void)jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */
    
	/* Step 8: Release JPEG decompression object */
	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);
    
	/* At this point you may want to check to see whether any corrupt-data
	 * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	 */

	/* And we're done! */
	fclose(infile);
	return 0;
}
