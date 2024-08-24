#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "get_image.h"

int get_image(const char *filename, image_t *image) {	
	int len = strlen(filename);

	if (!strcmp(filename + len - 3, "png")) {
		return get_image_png(filename, image);
	} else if (!strcmp(filename + len - 3, "jpg")) {
		return get_image_jpg(filename, image);
	} else {
		fprintf(stderr, "FATAL: Unknown extension on input file '%s'\n", filename);
		return -1;
	}
}
