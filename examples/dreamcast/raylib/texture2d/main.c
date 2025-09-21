#include <raylib/raylib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ASSETS "/rd"
//#define ASSETS "images"

typedef struct ImageCollection {
	Image *image_arr;
	int capacity;
	int used;
} ImageCollection;

void image_collection_init(ImageCollection *collection, int initialsize) {
	collection->image_arr = malloc(sizeof(Image) * initialsize);
	if (!collection->image_arr)
		return;
	collection->capacity = initialsize;
	collection->used = 0;
}

void image_collection_add(ImageCollection *collection, Image *img) {
	if (collection->capacity == collection->used) {
		collection->capacity <<= 1;
		collection->image_arr = realloc(collection->image_arr, sizeof(Image) * collection->capacity);
		if (!collection->image_arr)
			return;
	}
	memcpy(&collection->image_arr[collection->used], img, sizeof(Image));
	++collection->used;
}

uint32_t power_of_two(int dim) {
	// int is 32-bit.
	dim--;
	dim |= dim >> 1;
	dim |= dim >> 2;
	dim |= dim >> 4;
	dim |= dim >> 8;
	dim |= dim >> 16;
	dim++;
	return (uint32_t)dim;
}

int main() {
	InitWindow(640, 480, "Raylib Image Test");
	if (!IsWindowReady())
		return 1;
	SetTargetFPS(60);

	ImageCollection collection;
	image_collection_init(&collection, 10);

	// We're gonna load each file and blit it to the screen.
	DIR *dir = opendir(ASSETS);
	if (!dir) {
		printf("Directory load failure!");
		return 1;
	}

	struct dirent *at_dir;

	while ((at_dir = readdir(dir))) {
		char buf[NAME_MAX+5];
		strcpy(buf, ASSETS"/");
		strcat(buf, at_dir->d_name);
		Image img = LoadImage(buf);
		if (IsImageValid(img)) {
			// Resize the image to a power of 2 to convert it to a texture.

			// For the Dreamcast, Textures (in VRAM) must have dimensions
			// of a power of 2 to correctly draw.
			// So we resize the image canvas here to load the texture
			// specifically in dimensions of a power of 2.
			uint32_t width = power_of_two(img.width);
			uint32_t height = power_of_two(img.height);

			ImageResizeCanvas(&img, width, height, 0, 0, BLANK);

			image_collection_add(&collection, &img);
		}
	}

	closedir(dir);

	if (collection.used == 0)
		return -1; // Don't draw if empty selection.

	// Load the textures before the loop. Loading textures after BeginDrawing does not work.
	Texture2D *textures = malloc(sizeof(Texture2D) * collection.used);
	for (int i = collection.used; --i;) {
		textures[i] = LoadTextureFromImage(collection.image_arr[i]);
	}

	// Now we blit all the sprites to the screen repeatedly in random locations.
	while (1) {
		BeginDrawing();
		for (int i = collection.used; --i;) {
			if (IsTextureValid(textures[i])) {
				int x = rand() % 640, y = rand() % 480;
				DrawTexture(textures[i], x, y, WHITE);
			}
		}
		EndDrawing();
	}
	return 0;
}
