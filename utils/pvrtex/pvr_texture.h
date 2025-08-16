#pragma once

#include <math.h>
#include <stdbool.h>
#include "nvmath.h"
#include "pixel.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PVR_MAX_TEXTURE_WIDTH	1024
#define PVR_MAX_TEXTURE_HEIGHT	1024
#define PVR_MAX_MIPMAPS	(11)

#define CHANNEL_CNT_ARGB	4
#define VECTOR_W	2
#define VECTOR_H	2
#define VECTOR_AREA	(VECTOR_W * VECTOR_H)
#define VECTOR_SIZE	(CHANNEL_CNT_ARGB * VECTOR_AREA)

#define PVR_CODEBOOK_ENTRY_SIZE_BYTES	(8)
#define PVR_CODEBOOK_SIZE_BYTES	(2048)
#define PVR_FULL_CODEBOOK	(256)
#define PVR_4B_PALETTE_SIZE	(16)
#define PVR_8B_PALETTE_SIZE	(256)

typedef enum {
	PT_SIZE_8,
	PT_SIZE_16,
	PT_SIZE_32,
	PT_SIZE_64,
	PT_SIZE_128,
	PT_SIZE_256,
	PT_SIZE_512,
	PT_SIZE_1024,
} ptTextureSize;

typedef enum {
	//The values for following seven formats match up with the values used by the hardware
	PT_ARGB1555,
	PT_RGB565,
	PT_ARGB4444,
	PT_YUV,
	PT_NORMAL,
	PT_PALETTE_4B,
	PT_PALETTE_8B,
	
	PT_PSEUDO_FORMAT_START,
	//This is not a real PVR pixel format. It's a stand in for a YUV texture that is twiddled,
	//which is not encoded the same way as other twiddled formats.
	//It exists so that ConvertFromFormatToBGRA8888 can know that a texture is twiddled.
	PT_YUV_TWID = PT_PSEUDO_FORMAT_START,
	
	//Also not a real PVR format, used by ptConvertToTargetFormat to encode normals as texconv does
	PT_NORMAL_TEXCONV,
	
	PTE_START,
	//The following cannot be used with the functions defined in this header.
	//They are used by pte* functions in pvr_texture_encoder only
	PTE_ABGR8888,
	PTE_BUMP,	//Signals input is height map that needs to be converted to normal map
	PTE_AUTO,	//Selects RGB565, ARGB1555, or ARGB4444 automatically based on alpha of input image
	PTE_AUTO_YUV,	//Selects YUV, ARGB1555, or ARGB4444 automatically based on alpha of input image
} ptPixelFormat;
#define PT_PIXEL_OFFSET	PT_PALETTE_8B

//Returns the number of pixels in a codebook entry for a compressed texture of a given format
unsigned VectorArea(ptPixelFormat format);

size_t TotalMipSize(ptPixelFormat format, int vq, int level);
size_t UncompressedMipSize(unsigned w, unsigned h, ptPixelFormat format);
size_t MipMapOffset(ptPixelFormat format, int vq, int level);
size_t CalcTextureSize(int u, int v, ptPixelFormat format, int mipmap, int vq, int codebook_size_bytes);
void MakeTwiddled32(void *pix, int w, int h);
void MakeTwiddled16(void *pix, int w, int h);
void MakeTwiddled8(void *pix, int w, int h);
void MakeDetwiddled8(void *pix, int w, int h);
void MakeDetwiddled32(void *pix, int w, int h);
void DecompressVQ(const uint8_t *indicies, int index_cnt, const void *codebook, int cb_offset, void *dst);
unsigned MipLevels(int size);
bool IsValidStrideWidth(unsigned size);
float BytesPerPixel(ptPixelFormat format);
void ConvertFromFormatToBGRA8888(const void *src, int pixel_format, pxlABGR8888 *pal, unsigned w, unsigned h, pxlABGR8888 *dst);
void ptConvertToTargetFormat(const pxlABGR8888 *src, unsigned w, unsigned h, pxlABGR8888 *pal, size_t palsize, void *dst, ptPixelFormat pixel_format);
const char * ptGetPixelFormatString(unsigned format);

static inline unsigned ConvToYUV(pxlABGR8888 l, pxlABGR8888 r) {
	const float avgR = (l.r + r.r) / 2;
	const float avgG = (l.g + r.g) / 2;
	const float avgB = (l.b + r.b) / 2;

	//compute each pixel's Y
	int Y0 = CLAMP(0, (int)(0.299 * l.r + 0.587 * l.g + 0.114 * l.b), 255);
	int Y1 = CLAMP(0, (int)(0.299 * r.r + 0.587 * r.g + 0.114 * r.b), 255);
	
	int U = CLAMP(0, (int)(-0.169 * avgR - 0.331 * avgG + 0.4990 * avgB + 128), 255);
	int V = CLAMP(0, (int)( 0.499 * avgR - 0.418 * avgG - 0.0813 * avgB + 128), 255);

	unsigned yuv1 = ((uint8_t)Y0) << 8 | (uint8_t)U;
	unsigned yuv2 = ((uint8_t)Y1) << 8 | (uint8_t)V;
	
	return (yuv1 << 16) | yuv2;
}

//Writes two pixels, to dst[0] and dst[1]
static inline void ConvFromYUV(unsigned yuv1, unsigned yuv2, pxlABGR8888 *dst) {
	const int Y0 = (yuv1 & 0xFF00) >> 8;
	const int Y1 = (yuv2 & 0xFF00) >> 8;
	const int U = (int)(yuv1 & 0xFF) - 128;
	const int V = (int)(yuv2 & 0xFF) - 128;
	int r, g, b;

	r = CLAMP(0, (int)(Y0 + 1.375 * V), 255);
	g = CLAMP(0, (int)(Y0 - 0.34375 * U - 0.6875 * V), 255);
	b = CLAMP(0, (int)(Y0 + 1.71875 * U), 255);
	dst[0] = pxlSetABGR8888(pxlU8toF(r), pxlU8toF(g), pxlU8toF(b), 1);

	r = CLAMP(0, (int)(Y1 + 1.375 * V), 255);
	g = CLAMP(0, (int)(Y1 - 0.34375 * U - 0.6875 * V), 255);
	b = CLAMP(0, (int)(Y1 + 1.71875 * U), 255);
	dst[1] = pxlSetABGR8888(pxlU8toF(r), pxlU8toF(g), pxlU8toF(b), 1);
}

#ifdef __cplusplus
}
#endif
