
#ifndef __GET_IMAGE_H
#define __GET_IMAGE_H 

typedef struct image_t {
	int w;
	int h;
	int bpp;
	int stride;
	unsigned char *data;
} image_t;

int get_image(const char *filename, image_t *image);
int get_image_jpg(const char *filename, image_t *image);
int get_image_png(const char *filename, image_t *image);

#endif
