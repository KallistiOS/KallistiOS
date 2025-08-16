#include <assert.h>
#include <string.h>
#include <stdint.h>
#include "pvr_texture_encoder.h"
#include "stb_image.h"
#include "file_common.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

int LoadPalette(const char *fname, PvrTexEncoder *pte) {
	assert(fname);
	assert(pte);
	
	size_t max_colors = pte->pixel_format == PT_PALETTE_8B ? PVR_8B_PALETTE_SIZE : PVR_4B_PALETTE_SIZE;
	if (pte->palette_size)
		max_colors = pte->palette_size;
	assert(max_colors <= PVR_8B_PALETTE_SIZE);
	assert(max_colors > 0);
	
	const char *extension = strrchr(fname, '.');
	ErrorExitOn(extension == NULL, "Unknown file type for palette file '%s'\n", fname);
	unsigned colors_found = 0;
	unsigned colors_written = 0;
	
	if (strcasecmp(extension, ".pal") == 0) {
		void *data = NULL;
		size_t size = Slurp(fname, &data);
		
		
		ErrorExitOn(size < 12, ".PAL file is too short\n");
		ErrorExitOn(memcmp(data, "DPAL", 4) != 0, ".PAL has wrong fourcc\n");
		
		uint32_t color_count = ((uint32_t*)data)[1];
		pteLog(LOG_INFO, "Palette '%s' has %u colors\n", fname, color_count);
	
		size_t color_size_bytes = color_count * sizeof(pxlABGR8888);
		ErrorExitOn(color_count == 0, ".PAL file appears to contain no colors\n");
		ErrorExitOn(color_count > PVR_8B_PALETTE_SIZE, ".PAL file appears to contain more than 256 colors (%u found)\n", color_count);
		ErrorExitOn(size < (color_size_bytes + 8), ".PAL file is too short to contain %u colors\n", color_count);
		
		pxlARGB8888 *colors = data+8;
		SMART_ALLOC(&pte->palette, color_size_bytes);
		for(unsigned i = 0; i < color_count; i++) {
			pte->palette[i] = pxlConvertARGB8888toABGR8888(colors[i]);
		}
		colors_found = colors_written = color_count;
	} else {
		pteImage img;
		img.w = img.h = 0;
		img.channels = 4;
		img.pixels = (void*)stbi_load(fname, &img.w, &img.h, &img.channels, 4);
		ErrorExitOn(img.pixels == NULL,
			"Could not load image \"%s\" for palette source, exiting\n", fname);
		
		SMART_ALLOC(&pte->palette, PVR_8B_PALETTE_SIZE * sizeof(pxlABGR8888));
		
		struct { pxlABGR8888 key; char value; } *ht = NULL;
		unsigned wh = img.h * img.w;
		for(unsigned i = 0; i < wh; i++) {
			pxlABGR8888 p = img.pixels[i];
			if (hmgeti(ht, p) >= 0) {
				//Found color already
			} else {
				hmput(ht, p, 1);
				colors_found++;
				if (colors_written < max_colors)
					pte->palette[colors_written++] = p;
			}
		}
		
		free(img.pixels);
	}
	
	if (colors_found > max_colors) {
		pteLog(LOG_WARNING, "Found %u colors, only the first %u will be used\n", colors_found, max_colors);
		pte->palette_size = max_colors;
	} else {
		pteLog(LOG_INFO, "Using %u colors from palette from %s\n", pte->palette_size, fname);
	}
	
	pte->palette_size = colors_written < max_colors ? colors_written : max_colors;
	
	return 0;
}
