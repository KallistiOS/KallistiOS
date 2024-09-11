#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "pvr_texture.h"

#define make_mask(width) ((width != 0) ? (0xffffffffu >> (32 - (width))) : 0)

typedef struct {
	unsigned int x_mask, y_mask;
	unsigned int x_inc, y_inc;
} Morton2D;

#define M2DIncX(m2d, v) (((v) + (m2d).x_inc) & (m2d).x_mask)
#define M2DIncY(m2d, v) (((v) + (m2d).y_inc) & (m2d).y_mask)

#define BAD_PIXEL() assert((0 && "Bad pixel format"))

static void AssertPixelFormat(ptPixelFormat fmt) {
	assert((fmt >= PT_ARGB1555 && fmt <= PT_PALETTE_8B) || fmt == PT_YUV_TWID);
}

static inline Morton2D Morton2DInit(unsigned int x_bits, unsigned int y_bits) {
	Morton2D m2d;

	int shared = y_bits < x_bits ? y_bits : x_bits;
	int x_extra = x_bits - shared;
	int y_extra = y_bits - shared;

	shared = (shared-1)*2;
	m2d.x_mask = 0xAAAAAAAA & make_mask(shared);
	m2d.x_mask |= (make_mask(x_extra) << (shared));
	m2d.x_inc = 0x2 | ~m2d.x_mask;

	m2d.y_mask = 0x55555555 & make_mask(shared);
	m2d.y_mask |= (make_mask(y_extra) << shared);
	m2d.y_inc = 0x1 | ~m2d.y_mask;

	return m2d;
}

float BytesPerPixel(ptPixelFormat format) {
	switch(format) {
	case PT_ARGB1555:
	case PT_RGB565:
	case PT_ARGB4444:
	case PT_YUV:
	case PT_NORMAL:
	case PT_YUV_TWID:
		return 2;
	case PT_PALETTE_8B:
		return 1;
	case PT_PALETTE_4B:
		return 0.5;
	default:
		BAD_PIXEL();
	}
}
size_t UncompressedMipSize(unsigned w, unsigned h, ptPixelFormat format) {
	switch(format) {
	case PT_ARGB1555:
	case PT_RGB565:
	case PT_ARGB4444:
	case PT_YUV:
	case PT_YUV_TWID:
	case PT_NORMAL:
		return w*h*2;
	case PT_PALETTE_8B:
		return w*h;
	case PT_PALETTE_4B:
		return w*h/2;
	default:
		BAD_PIXEL();
	}
}
unsigned VectorArea(ptPixelFormat format) {
	switch(format) {
	case PT_ARGB1555:
	case PT_RGB565:
	case PT_ARGB4444:
	case PT_YUV:
	case PT_YUV_TWID:
	case PT_NORMAL:
		return 4;
	case PT_PALETTE_8B:
		return 8;
	case PT_PALETTE_4B:
		return 16;
	default:
		BAD_PIXEL();
	}
}

size_t TotalMipSize(ptPixelFormat format, int vq, int level) {
	AssertPixelFormat(format);
	return level ? CalcTextureSize(1<<(level-1), 1<<(level-1), format, 1, vq, 0) : 0;
}

size_t CalcTextureSize(int u, int v, ptPixelFormat format, int mipmap, int vq, int codebook_size_bytes) {
	AssertPixelFormat(format);

	if (mipmap)
		v = u;

	size_t texsize = u * v;

	if (mipmap)
		texsize = texsize *4/3 + 3;

	if ((unsigned)format <= PT_NORMAL || format == PT_YUV_TWID)
		texsize *= 2;
	else if (format == PT_PALETTE_4B)
		texsize /= 2;
	else if (format == PT_PALETTE_8B)
		;
	else
		BAD_PIXEL();

	if (vq)
		texsize = (texsize+7) / 8 + codebook_size_bytes;

	return texsize;
}

size_t MipMapOffset(ptPixelFormat format, int vq, int level) {
	AssertPixelFormat(format);

	static size_t const ofs[] = {
		0x00006,	//1	6
		0x00008,	//2	8
		0x00010,	//4	16
		0x00030,	//8	48
		0x000B0,	//16	176
		0x002B0,	//32	688
		0x00AB0,	//64	2736
		0x02AB0,	//128	10928
		0x0AAB0,	//256	42696
		0x2AAB0,	//512	174768
		0xAAAB0,	//1024	699056
	};

	assert(level >= 0);
	assert(level <= 10);

	size_t ret = ofs[level];

	if (vq)
		return ret/8;

	switch(format) {
	case PT_ARGB1555:
	case PT_RGB565:
	case PT_ARGB4444:
	case PT_YUV:
	case PT_YUV_TWID:
	case PT_NORMAL:
		return ret;
	case PT_PALETTE_4B:
		return ret/4;
	case PT_PALETTE_8B:
		return ret/2;
	default:
		BAD_PIXEL();
	}

	return 0;
}

void MakeTwiddled8(void *pix, int w, int h) {
	char *cpy = malloc(w*h), *dst = pix;
	char *src = cpy;
	memcpy(cpy, pix, w*h);

	Morton2D m2d = Morton2DInit(__builtin_ffs(w), __builtin_ffs(h));

	int xmorton = 0, ymorton = 0;
	int i, j;
	for(j = 0; j < h; j++) {
		for(i = 0, xmorton = 0; i < w; i++) {
			dst[xmorton | ymorton] = *src++;
			xmorton = M2DIncX(m2d, xmorton);
		}
		ymorton = M2DIncY(m2d, ymorton);
	}
	free(cpy);
}

void MakeTwiddled16(void *pix, int w, int h) {
	uint16_t *cpy = malloc(w*h*2), *dst = pix;
	uint16_t *src = cpy;
	memcpy(cpy, pix, w*h*2);

	Morton2D m2d = Morton2DInit(__builtin_ffs(w), __builtin_ffs(h));

	int xmorton = 0, ymorton = 0;
	int i, j;
	for(j = 0; j < h; j++) {
		for(i = 0, xmorton = 0; i < w; i++) {
			dst[xmorton | ymorton] = *src++;
			xmorton = M2DIncX(m2d, xmorton);
		}
		ymorton = M2DIncY(m2d, ymorton);
	}
	free(cpy);
}

void MakeTwiddled32(void *pix, int w, int h) {
	uint32_t *cpy = malloc(w*h*4), *dst = pix;
	uint32_t *src = cpy;
	memcpy(cpy, pix, w*h*4);

	Morton2D m2d = Morton2DInit(__builtin_ffs(w), __builtin_ffs(h));

	int xmorton = 0, ymorton = 0;
	int i, j;
	for(j = 0; j < h; j++) {
		for(i = 0, xmorton = 0; i < w; i++) {
			dst[xmorton | ymorton] = *src++;
			xmorton = M2DIncX(m2d, xmorton);
		}
		ymorton = M2DIncY(m2d, ymorton);
	}
	free(cpy);
}

void MakeDetwiddled32(void *pix, int w, int h) {
	uint32_t *cpy = malloc(w*h*4), *dst = pix;
	memcpy(cpy, pix, w*h*4);

	Morton2D m2d = Morton2DInit(__builtin_ffs(w), __builtin_ffs(h));

	int xmorton = 0, ymorton = 0;
	int i, j;
	for(j = 0; j < h; j++) {
		for(i = 0, xmorton = 0; i < w; i++) {
			*dst++ = cpy[xmorton | ymorton];
			xmorton = M2DIncX(m2d, xmorton);
		}
		ymorton = M2DIncY(m2d, ymorton);
	}
	free(cpy);
}
void MakeDetwiddled8(void *pix, int w, int h) {
	char *cpy = malloc(w*h), *dst = pix;
	memcpy(cpy, pix, w*h);

	Morton2D m2d = Morton2DInit(__builtin_ffs(w), __builtin_ffs(h));

	int xmorton = 0, ymorton = 0;
	int i, j;
	for(j = 0; j < h; j++) {
		for(i = 0, xmorton = 0; i < w; i++) {
			*dst++ = cpy[xmorton | ymorton];
			xmorton = M2DIncX(m2d, xmorton);
		}
		ymorton = M2DIncY(m2d, ymorton);
	}
	free(cpy);
}

void DecompressVQ(const uint8_t *indicies, int index_cnt, const void *codebook, int cb_offset, void *dst) {
	const uint64_t *cb = codebook;
	uint64_t *d = dst;
	int i;
	for(i = 0; i < index_cnt; i++) {
		d[i] = cb[indicies[i] + cb_offset];
	}
}

void ConvertFromFormatToBGRA8888(const void *src, int pixel_format, pxlABGR8888 *pal, unsigned w, unsigned h, pxlABGR8888 *dst) {
	AssertPixelFormat(pixel_format);
	assert(src);
	assert(dst);

	size_t cnt = w*h;
	switch(pixel_format) {
		case PT_RGB565: {
			const pxlRGB565 *psrc = src;
			for(size_t i = 0; i < cnt; i++)
				dst[i] = pxlConvertRGB565toABGR8888(psrc[i]);
		} break;
		case PT_ARGB4444: {
			const pxlARGB4444 *psrc = src;
			for(size_t i = 0; i < cnt; i++)
				dst[i] = pxlConvertARGB4444toABGR8888(psrc[i]);
		} break;
		case PT_ARGB1555: {
			const pxlARGB1555 *psrc = src;
			for(size_t i = 0; i < cnt; i++)
				dst[i] = pxlConvertARGB1555toABGR8888(psrc[i]);
		} break;
		case PT_NORMAL: {
			const uint16_t *psrc = src;
			for(size_t i = 0; i < cnt; i++) {
				dst[i] = pxlSphericaltoABGR8888(psrc[i]);
			
			}
		} break;
		case PT_YUV_TWID: {
			//YUV pixels always come in pairs
			assert((cnt % 2) == 0);
		
			const uint16_t *psrc = src;
			for(size_t i = 0; i < cnt; i += 4) {
				pxlABGR8888 dec[4];
				ConvFromYUV(psrc[i+0], psrc[i+2], dec);
				ConvFromYUV(psrc[i+1], psrc[i+3], dec+2);
				dst[i+0] = dec[0];
				dst[i+1] = dec[2];
				dst[i+2] = dec[1];
				dst[i+3] = dec[3];
			}
		} break;
		case PT_YUV: {
			//YUV pixels always come in pairs
			assert((cnt % 2) == 0);
		
			const uint16_t *psrc = src;
			for(size_t i = 0; i < cnt; i += 2) {
				ConvFromYUV(psrc[i+0], psrc[i+1], dst + i);
			}
		} break;
		case PT_PALETTE_8B: {
			assert(pal);
			const uint8_t *psrc = src;
			for(size_t i = 0; i < cnt; i++) {
				dst[i] = pal[psrc[i]];
			
			}
		} break;
		case PT_PALETTE_4B: {
			assert(pal);
			assert((cnt & 1) == 0);
			assert(cnt > 1);
			const uint8_t *psrc = src;
			for(size_t i = 0; i < cnt/2; i++) {
				dst[i*2+0] = pal[psrc[i] & 0xf];
				dst[i*2+1] = pal[psrc[i] >> 4];
			
			}
		} break;
		default:
			BAD_PIXEL();
	}
}

void ptConvertToTargetFormat(const pxlABGR8888 *src, unsigned w, unsigned h, pxlABGR8888 *pal, size_t palsize, void *dst, ptPixelFormat pixel_format) {
	AssertPixelFormat(pixel_format);
	assert(src);
	assert(dst);
	size_t cnt = w*h;
	switch(pixel_format) {
	case PT_RGB565: {
		pxlRGB565 *pdst = dst;
		for(size_t i = 0; i < cnt; i++)
			pdst[i] = pxlConvertABGR8888toRGB565(src[i]);
	} break;
	case PT_ARGB4444: {
		pxlARGB4444 *pdst = dst;
		for(size_t i = 0; i < cnt; i++)
			pdst[i] = pxlConvertABGR8888toARGB4444(src[i]);
	} break;
	case PT_ARGB1555: {
		pxlARGB1555 *pdst = dst;
		for(size_t i = 0; i < cnt; i++)
			pdst[i] = pxlConvertABGR8888toARGB1555(src[i]);
	} break;
	case PT_NORMAL: {
		uint16_t *pdst = dst;
		for(size_t i = 0; i < cnt; i++) {
			pdst[i] = pxlRGBtoSpherical(src[i].r, src[i].g, src[i].b);
		
		}
	} break;
	case PT_YUV: {
		//Untwiddled YUV
		uint16_t *pdst = dst;
	
		//YUV always encodes pairs
		assert((cnt % 2) == 0);
		for(size_t i = 0; i < cnt; i+=2) {
			uint32_t yuv = ConvToYUV(src[i+0], src[i+1]);
			pdst[i+0] = yuv>>16;
			pdst[i+1] = yuv;
		}
	} break;
	case PT_YUV_TWID: {
		//Twiddled YUV
		uint16_t *pdst = dst;
	
		//YUV always encodes pairs
		assert((cnt % 2) == 0);
		for(size_t i = 0; i < cnt; i+=4) {
			uint32_t yuv_top = ConvToYUV(src[i+0], src[i+2]);
			uint32_t yuv_bottom = ConvToYUV(src[i+1], src[i+3]);
			pdst[i+0] = yuv_top>>16;
			pdst[i+1] = yuv_bottom>>16;
			pdst[i+2] = yuv_top;
			pdst[i+3] = yuv_bottom;
		}
	} break;
	case PT_PALETTE_8B: {
		assert(pal);
		assert(palsize <= 256);
		assert(palsize > 0);
		uint8_t *pdst = dst;
		for(size_t i = 0; i < cnt; i++) {
			pdst[i] = pxlFindClosestColor(src[i], pal, palsize);
		
		}
	} break;
	case PT_PALETTE_4B: {
		assert(pal);
		assert(palsize <= 16);
		assert(palsize > 0);
		uint8_t *pdst = dst;
		for(size_t i = 0; i < cnt/2; i++) {
			pdst[i] = (pxlFindClosestColor(src[i*2+1], pal, palsize) << 4) | pxlFindClosestColor(src[i*2], pal, palsize);
		
		}
	} break;
	default:
		BAD_PIXEL();
	}
}

bool IsValidStrideWidth(unsigned size) {
	if (size > 1024 || size == 0)
		return 0;
	if (size == 8 || size == 16)
		return 1;
	if ((size % 32) == 0)
		return 1;
	return 0;
}

unsigned MipLevels(int size) {
	if (size == 1024)
		return 11;
	else if (size == 512)
		return 10;
	else if (size == 256)
		return 9;
	else if (size == 128)
		return 8;
	else if (size == 64)
		return 7;
	else if (size == 32)
		return 6;
	else if (size == 16)
		return 5;
	else if (size == 8)
		return 4;
	else if (size == 4)
		return 3;
	else if (size == 2)
		return 2;
	else if (size == 1)
		return 1;
	else
		return 0;
	assert(0);
}

const char * ptGetPixelFormatString(unsigned format) {
	const char *name[] = {
		"ARGB1555", "RGB565", "ARGB4444", "YUV422", "NORMAL", "PAL4BPP", "PAL8BPP", "INVALID"
	};
	if (format > PT_YUV_TWID)
		format = PT_YUV;
	if (format > 7)
		format = 7;
	return name[format];
}
