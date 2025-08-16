#pragma once

#include "pvr_texture.h"

//Dithering nearest converters
void tdDNearestARGB4444(const float *sample, int sample_size, const pxlABGR8888 *palette, size_t palette_size, float *nearest_dst);
void tdDNearestARGB1555(const float *sample, int sample_size, const pxlABGR8888 *palette, size_t palette_size, float *nearest_dst);
void tdDNearestRGB565(const float *sample, int sample_size, const pxlABGR8888 *palette, size_t palette_size, float *nearest_dst);

typedef void (*dithFindNearest)(const float *sample, int sample_size, const pxlABGR8888 *palette, size_t palette_size, float *nearest_dst);

dithFindNearest pteGetFindNearest(ptPixelFormat format);
void pteDither(const unsigned char *src, unsigned w, unsigned h, unsigned channels, float dither_amt,
	dithFindNearest nearest, const pxlABGR8888 *palette, size_t palette_size, void *dst, int dst_pixel_format);
