#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "./pixel.h"
#include "./pvr_texture_encoder.h"
#include "./pvr_texture.h"
#include "./tddither.h"



void pteDNearestARGB4444(const float *sample, int sample_size, const pxlABGR8888 *palette, size_t palette_size, float *nearest_dst) {
	(void)palette;
	(void)sample_size;
	v4f t = v4DivS(v4Float(v4Int(v4AddS(v4MulS(v4Get(sample), 16), 0.5))), 16);
	nearest_dst[0] = t.x;
	nearest_dst[1] = t.y;
	nearest_dst[2] = t.z;
	nearest_dst[3] = t.w;
}
void pteDNearestARGB1555(const float *sample, int sample_size, const pxlABGR8888 *palette, size_t palette_size, float *nearest_dst) {
	(void)palette; (void)sample_size;
	v4f t = v4Mul(v4Get(sample), v4Set(32,32,32,1));  // Scale by bit depth
	t = v4Float(v4Int(v4AddS(t, 0.5)));	 // Round to nearest
	t = v4Div(t, v4Set(32,32,32,1));	 // Unscale by bit depth

	nearest_dst[0] = t.x;
	nearest_dst[1] = t.y;
	nearest_dst[2] = t.z;
	nearest_dst[3] = t.w;
}

void pteDNearestRGB565(const float *sample, int sample_size, const pxlABGR8888 *palette, size_t palette_size, float *nearest_dst) {
	(void)palette; (void)sample_size;
	v4f t = v4Mul(v4Get(sample), v4Set(32,64,32,0));	//Scale by bit depth
	t = v4Float(v4IntRnd(t));	//Round to nearest
	t = v4Div(t, v4Set(32,64,32,0.5));	//Unscale by bit depth

	nearest_dst[0] = t.x;
	nearest_dst[1] = t.y;
	nearest_dst[2] = t.z;
	nearest_dst[3] = t.w;
}
void pteDNearestNorm(const float *sample, int sample_size, const pxlABGR8888 *palette, size_t palette_size, float *nearest_dst) {
	int norm = pxlRGBtoSpherical(sample[0] * 255.0f, sample[1] * 255.0f, sample[2] * 255.0f);

	pxlABGR8888 n = pxlSphericaltoABGR8888(norm);

	nearest_dst[0] = pxlU8toF(n.r);
	nearest_dst[1] = pxlU8toF(n.g);
	nearest_dst[2] = pxlU8toF(n.b);
	nearest_dst[3] = 1;
}

void pteDNearest8BPP(const float *sample, int sample_size, const pxlABGR8888 *palette, size_t palette_size, float *nearest_dst) {
	v4f cf = v4MulS(v4Get(sample), 1);
	pxlABGR8888 c = pxlSetABGR8888(v4Pass(cf));
	unsigned idx = pxlFindClosestColor(c, palette, palette_size);
	pxlABGR8888 *nc = ((pxlABGR8888*)palette) + idx;
	nearest_dst[0] = nc->r / 255.;
	nearest_dst[1] = nc->g / 255.;
	nearest_dst[2] = nc->b / 255.;
	nearest_dst[3] = nc->a / 255.;
}


void pteConvertFPtoARGB4444(const float *img, unsigned w, unsigned h, unsigned channels, const pxlABGR8888 *palette, size_t palette_size, void * restrict dst) {
	assert(channels == 4);
	assert(dst);
	assert(img);

	pxlARGB4444 *cdst = dst;
	for(unsigned y = 0; y < h; y++) {
		for(unsigned x = 0; x < w; x++) {
			int ofs = (y*w+x)*channels;
			cdst[y*w+x] = pxlSetARGB4444(img[ofs + 0], img[ofs + 1], img[ofs + 2], img[ofs + 3]);
		}
	}
}
void pteConvertFPtoARGB1555(const float *img, unsigned w, unsigned h, unsigned channels, const pxlABGR8888 *palette, size_t palette_size, void * restrict dst) {
	assert(channels == 4);
	assert(dst);
	assert(img);

	pxlARGB1555 *cdst = dst;
	for(unsigned y = 0; y < h; y++) {
		for(unsigned x = 0; x < w; x++) {
			int ofs = (y*w+x)*channels;
			cdst[y*w+x] = pxlSetARGB1555(img[ofs + 0], img[ofs + 1], img[ofs + 2], img[ofs + 3]);
		}
	}
}
void pteConvertFPtoRGB565(const float *img, unsigned w, unsigned h, unsigned channels, const pxlABGR8888 *palette, size_t palette_size, void * restrict dst) {
	assert(channels == 4);
	assert(dst);
	assert(img);

	pxlRGB565 *cdst = dst;
	for(unsigned y = 0; y < h; y++) {
		for(unsigned x = 0; x < w; x++) {
			int ofs = (y*w+x)*channels;
			cdst[y*w+x] = pxlSetRGB565(img[ofs + 0], img[ofs + 1], img[ofs + 2]);
		}
	}
}

void pteConvertFPtoABGR8888(const float *img, unsigned w, unsigned h, unsigned channels, const pxlABGR8888 *palette, size_t palette_size, void * restrict dst) {
	assert(channels == 4);
	assert(dst);
	assert(img);

	pxlABGR8888 *cdst = dst;
	for(unsigned y = 0; y < h; y++) {
		for(unsigned x = 0; x < w; x++) {
			int ofs = (y*w+x)*channels;
			cdst[y*w+x] = pxlSetABGR8888(img[ofs + 0], img[ofs + 1], img[ofs + 2], img[ofs + 3]);
		}
	}
}

#define MAX_CHANNELS	(4*4*4)
#define VGAMMA	((float)(1))
#define RVGAMMA	(1.0f/VGAMMA)
void pteDither(const unsigned char *src, unsigned w, unsigned h, unsigned channels, float dither_amt, dithFindNearest nearest, const pxlABGR8888 *palette, size_t palette_size, void *dst, int dst_pixel_format) {
	assert(src);
	assert(dst);
	assert(channels < MAX_CHANNELS);

	//Convert image to floats
	float *imgf = malloc(sizeof(float) * w * h * channels);
	//printf("Dither %ux%ux%u\n",w,h,channels);
	//printf("%zu -> %p\n",sizeof(float) * w * h * channels, imgf);
	for(unsigned y = 0; y < h; y++) {
		for(unsigned x = 0; x < w; x++) {
			int ofs = (y*w+x)*channels;
			for(unsigned c = 0; c < channels; c++) {
				//~ imgf[ofs+c] = src[ofs+c] / 255.0f;
				imgf[ofs+c] = pow(src[ofs+c] / 255.0f, VGAMMA);
			}
		}
	}

	if (dither_amt != 0) {
		//Dither floating point image
		float near[4*4*4];
		float err[4*4*4];
		for(unsigned y = 0; y < h; y++) {
			for(unsigned x = 0; x < w; x++) {
				float *cur = imgf + (y*w+x)*channels;
				//~ nearest(cur, channels, palette, cur);
				//~ continue;
				nearest(cur, channels, palette, palette_size, &near[0]);
				//~ continue;
				for(int i = 0; i < channels; i++) {
					err[i] = CLAMP(0, cur[i] - near[i], 1);
					cur[i] = near[i];
				}
			
				/*
					 .0
					123
				*/
	#define diffuse(xo,yo, weight) do {\
		if ((x+(xo)) < w && (int)(x+(xo)) >= 0 && (y+(yo)) < h) \
			for(int i = 0; i < channels; i++) \
				cur[(w*(yo)+(xo)) * channels + i] += err[i] * (weight); \
		} while(0)
		
			if (1) {
				diffuse(1, 0, 7/16. * dither_amt);
				diffuse(-1, 1, 3/16. * dither_amt);
				diffuse(0, 1, 5/16. * dither_amt);
				diffuse(1, 1, 1/16. * dither_amt);
			} else {
				diffuse( 1, 0, 8/42. * dither_amt);
				diffuse( 2, 0, 4/42. * dither_amt);
			
				diffuse(-2, 1, 2/42. * dither_amt);
				diffuse(-1, 1, 4/42. * dither_amt);
				diffuse( 0, 1, 8/42. * dither_amt);
				diffuse( 1, 1, 4/42. * dither_amt);
				diffuse( 2, 1, 2/42. * dither_amt);
			
				diffuse(-2, 2, 1/42. * dither_amt);
				diffuse(-1, 2, 2/42. * dither_amt);
				diffuse( 0, 2, 4/42. * dither_amt);
				diffuse( 1, 2, 2/42. * dither_amt);
				diffuse( 2, 2, 1/42. * dither_amt);
			}
			
			}
		}
	}

	//Undo gamma correction
	for(unsigned y = 0; y < h; y++) {
		for(unsigned x = 0; x < w; x++) {
			unsigned ofs = (y*w+x)*channels;
			for(unsigned c = 0; c < channels; c++) {
				//~ imgf[ofs+c] = pow(CLAMP(0, imgf[ofs+c], 1), RVGAMMA);
				imgf[ofs+c] = pow(imgf[ofs+c], RVGAMMA);
			}
		}
	}

	switch(dst_pixel_format) {
	case PTE_ARGB4444: pteConvertFPtoARGB4444(imgf, w, h, channels, palette, palette_size, dst); break;
	case PTE_ARGB1555: pteConvertFPtoARGB1555(imgf, w, h, channels, palette, palette_size, dst); break;
	case PTE_RGB565: pteConvertFPtoRGB565(imgf, w, h, channels, palette, palette_size, dst); break;
	case PTE_ABGR8888: pteConvertFPtoABGR8888(imgf, w, h, channels, palette, palette_size, dst); break;
	}

	free(imgf);
}

dithFindNearest pteGetFindNearest(ptePixelFormat format) {
	static const dithFindNearest tbl[] = {
		&pteDNearestARGB1555,
		&pteDNearestRGB565,
		&pteDNearestARGB4444,
		NULL,
		&pteDNearestNorm,
		&pteDNearest8BPP,
		&pteDNearest8BPP,
	};

	assert(format >= PTE_ARGB1555 && format <= PTE_PALETTE_8B && format != PTE_YUV);

	return tbl[format];
}
