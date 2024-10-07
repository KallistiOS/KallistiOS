#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

/* TODO:
 * With this utility included, there are now four utilities
 * (the other three being "vqenc", "kmgenc" and "dcbumpgen") using
 * common texture operations such as twiddling and loading of png
 * and jpeg images.
 * Maybe it is time to break that stuff out into its own library
 * or create a megatool consisting of all the tree tools in one
 * executable.
 */
#include "get_image.h"

void encode_hemi_data(uint8_t *raw_xyz_data, int w, int h,
	uint8_t *raw_hemi_data, short *twidbuffer);

/* twiddling stuff copied from kmgenc.c */
#define TWIDTAB(x) ( (x&1)|((x&2)<<1)|((x&4)<<2)|((x&8)<<3)|((x&16)<<4)| \
	((x&32)<<5)|((x&64)<<6)|((x&128)<<7)|((x&256)<<8)|((x&512)<<9) )
#define TWIDOUT(x, y) ( TWIDTAB((y)) | (TWIDTAB((x)) << 1) )
#define MIN(a, b) ( (a)<(b)? (a):(b) )

#define ERROR(...) { fprintf(stderr, __VA_ARGS__); goto cleanup; }

static void printUsage(void)
{
	printf("dchemigen - Dreamcast hemisphere normal map generator v1.0\n");
	printf("Copyright (c) 2024 Jason Martin\n");
	printf("usage: dchemigen <infile.png/.jpg> optional <outfile.raw>\n");
	printf("Outputs <infile.raw> unless output filename is specified\n");
}

static int isPowerOfTwo(unsigned x)
{
	return x && !(x & (x-1));
}


int main(int argc, char **argv)
{
	uint8_t *raw_hemi_data = NULL;
	short *twidbuffer = NULL;

	FILE *fp = NULL;
	image_t img = {0};

	if (argc < 2) {
		printUsage();
		return 0;
	}

	if (get_image(argv[1], &img) < 0) {
		ERROR("Cannot open %s\n", argv[1]);
	}

	char *fn2;
	if (argc == 2) {
		int fn_len = strlen(argv[1]);
		fn2 = malloc(fn_len + 1);
		memset(fn2, 0, fn_len + 1);
		strcpy(fn2, argv[1]);
		int in_extension = 0;
		for(int i=0;i<fn_len;i++) {
			if (!in_extension) {
				if(fn2[i] == '.') {
					in_extension = 1;
				}
			} else {
				if(in_extension == 1) {
					fn2[i] = 'r';
					in_extension++;
				} else if (in_extension == 2) {
					fn2[i] = 'a';
					in_extension++;
				} else if (in_extension == 3) {
					fn2[i] = 'w';
					break;
				}
			}
		}
	} else {
		fn2 = argv[2];
	}

	fp = fopen(fn2, "wb");
	if (NULL == fp) {
		ERROR("Cannot open file %s!\n", argv[4]);
	}

	raw_hemi_data = malloc(2 * img.w * img.h);
	if (NULL == raw_hemi_data) {
		ERROR("Cannot allocate memory for image data!\n");
	}

	twidbuffer = malloc(2 * img.w * img.h);
		if (NULL == twidbuffer) {
		ERROR("Cannot allocate memory for twiddle buffer!\n");
	}

	if (!isPowerOfTwo(img.w) || !isPowerOfTwo(img.h)) {
		ERROR("Image dimensions %ux%u are not a power of two!\n", img.w, img.h);
	}

	encode_hemi_data(img.data, img.w, img.h, raw_hemi_data, twidbuffer);

	if (fwrite(twidbuffer, 2 * img.w * img.h, 1, fp) != 1) {
		ERROR("Cannot write twiddle buffer!\n");
	}

cleanup:
	if (fp) fclose(fp);
	if (raw_hemi_data) free(raw_hemi_data);
	if (twidbuffer) free(twidbuffer);
	if (img.data) free(img.data);

	return 0;
}

#define rescale(x) ((((double)(x) / 255.0) * 2.0) - 1.0)

void encode_hemi_data(uint8_t *raw_xyz_data, int w, int h,
	uint8_t *raw_hemi_data, short *twidbuffer)
{
	double x, y, z;
	double a, e;
	int outa, oute;
	double lenxy2;

	int dest = 0;
	int source = 1;
	int ih;
	int iw;
	for (ih = 0; ih < h; ih++) {
		for (iw = 0; iw < w; iw++, source += 4) {
			x = rescale(raw_xyz_data[source    ]);
			y = rescale(raw_xyz_data[source + 1]);
			z = rescale(raw_xyz_data[source + 2]);

			// flip y
			y = -y;

			lenxy2 = sqrt((x*x) + (y*y));

			// compute the azimuth angle
			a = atan2(y , x);

			// compute the elevation angle
			e = atan2(lenxy2, z);

			// a 0 - 2PI
			while (a < 0.0) {
				a += (M_PI * 2.0);
			}

			// e 0 - PI/2
			if (e < 0.0) e = 0.0;
			if (e > (M_PI / 2.0)) e = M_PI / 2.0;

			outa = (int)(255 * (a / (M_PI * 2.0)));
			oute = (int)(255 * (1.0 - (e / (M_PI / 2.0))));

			raw_hemi_data[dest] = outa; dest += 1;
			raw_hemi_data[dest] = oute; dest += 1;
		}
	}

	/* twiddle code based on code from kmgenc.c */
	int min = MIN(w, h);
	int mask = min-1;
	short *sbuffer = (short*) raw_hemi_data;
	for (int y=0; y<h; y++) {
		int yout = y;
		for (int x=0; x<w; x++) {
			twidbuffer[TWIDOUT(x&mask, yout&mask) +
				(x/min + yout/min)*min*min] = sbuffer[y*w+x];
		}
	}
}
