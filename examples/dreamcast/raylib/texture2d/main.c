#include <raylib/raylib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ASSETS "/rd"
//#define ASSETS "images"

/**
 * Functions for handling a custom vector of images.
 */
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

void image_collection_cleanup(ImageCollection *collection) {
	for (int i = collection->used; i--;) {
		UnloadImage(collection->image_arr[i]);
	}
	free(collection->image_arr);
}
/**
 * End custom vector
 */

/**
 * Structure for creating a circular buffer of textures.
 */
typedef struct {
	Texture *elem;
	int x;
	int y;
} ScreenElement;

typedef struct {
	ScreenElement *scr;
	int at;
	int len;
} ElementBuffer;

void buffer_add_element(ElementBuffer *buf, Texture *elem, int x, int y) {
	buf->scr[buf->at].elem = elem;
	buf->scr[buf->at].x = x;
	buf->scr[buf->at].y = y;
	buf->at = (buf->at + 1) % buf->len;
}

ElementBuffer *buffer_init(int len) {
	ElementBuffer *buf = malloc(sizeof(ElementBuffer));
	buf->scr = calloc(sizeof(ScreenElement), len);
	buf->at = 0;
	buf->len = len;
	return buf;
}

void buffer_cleanup(ElementBuffer *buf) {
	free(buf->scr);
	free(buf);
}
/**
 * End circular buffer
 */

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
		// Skip over . and .. filesystem directory structure.
		if (strcmp(at_dir->d_name, ".") == 0 || strcmp(at_dir->d_name, "..") == 0)
			continue;
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
	for (int i = collection.used; i--;) {
		textures[i] = LoadTextureFromImage(collection.image_arr[i]);
		// Once textures are loaded, you can free Images.
		// In our case, we have them in a data structure, so we wait until after the loop.
	}

	// Since we clean up the images, don't reference data that probably isn't right.
	int texture_len = collection.used;

	image_collection_cleanup(&collection);

	// 400 is arbitrary. Should be enough for images to stay for a decent amount of time.
	ElementBuffer *buf = buffer_init(400);

	// Now we blit all the sprites to the screen repeatedly in random locations.
	while (!WindowShouldClose()) {
		for (int i = texture_len; i--;) {
			if (IsTextureValid(textures[i])) {
				/* By subtracting the dimensions of the texture from the modulo,
				 * we can ensure that the sprites appear on the screen.
				 * Due to canvas resizing, this will make some sprites not draw
				 * all the way in the corner, but this is better behavior
				 * than drawing mostly off the screen.
				 */
				int x = rand() % (640 - textures[i].width),
				    y = rand() % (480 - textures[i].height);
				// Add to the circular buffer
				buffer_add_element(buf, &textures[i], x, y);
			}
		}
		BeginDrawing();
		// We loop like this so it draws from oldest entry to newest entry.
		for (int i = buf->at, first = 1; first == 1 || i != buf->at; first = 0, i = (i + 1) % buf->len) {
			if (buf->scr[i].elem != NULL) {
				DrawTexture(*buf->scr[i].elem, buf->scr[i].x, buf->scr[i].y, WHITE);
			}
		}
		EndDrawing();

		// Check the inputs for an exit combination of ABXYStart on every controller port
		// A more complicated program would likely want to decouple input checks from framerate,
		// which this example accomplishes by putting the input check in the same loop as drawing.
		for (int i = 0; i < 4; ++i) {
			// Assuming gamepad 0 is the primary controller
			if(IsGamepadAvailable(i)){
				int a_pressed = IsGamepadButtonDown(i, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
				int b_pressed = IsGamepadButtonDown(i, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
				int x_pressed = IsGamepadButtonDown(i, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
				int y_pressed = IsGamepadButtonDown(i, GAMEPAD_BUTTON_RIGHT_FACE_UP);
				int start_pressed = IsGamepadButtonDown(i, GAMEPAD_BUTTON_MIDDLE_RIGHT);

				// Now we do the check. We don't really care what controller presses the combo,
				// only that some controller presses the combo.
				if (a_pressed && b_pressed && x_pressed && y_pressed && start_pressed) {
					// I think this directly closes the window, which should escape the loop
					CloseWindow();
				}
			}
		}
	}

	// Cleanup. Even thought we shouldn't reach here under normal circumstances.
	buffer_cleanup(buf);
	for (int i = texture_len; i--;) {
		UnloadTexture(textures[i]);
	}
	CloseWindow();

	return 0;
}
