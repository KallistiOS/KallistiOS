#pragma once

#include <stdint.h>
#include <assert.h>
#include "mycommon.h"
#include "nvmath.h"

typedef union {
	int rgba[4];
	struct {
		int r, g, b, a;
	};
	v4i v;
} pxlRGBA32;

typedef union {
	//Assumes little endian
	uint32_t argb;
	struct {
		uint8_t b, g, r, a;
	};
} pxlARGB8888;

typedef union {
	//Assumes little endian
	uint32_t rgba;
	struct {
		uint8_t a, b, g, r;
	};
} pxlRGBA8888;

typedef union {
	//Assumes little endian
	uint32_t abgr;
	struct {
		uint8_t r,g,b,a;
	};
} pxlABGR8888;

typedef struct {
	uint8_t r, g, b;
} pxlRGB888;

typedef union {
	uint16_t v;
	struct {
		uint16_t b : 5;
		uint16_t g : 6;
		uint16_t r : 5;
	};
} pxlRGB565;

typedef union {
	uint16_t v;
	struct {
		uint16_t b : 5;
		uint16_t g : 5;
		uint16_t r : 5;
		uint16_t a : 1;
	};
} pxlARGB1555;

typedef union {
	uint16_t v;
	struct{
		uint16_t b : 4;
		uint16_t g : 4;
		uint16_t r : 4;
		uint16_t a : 4;
	};
} pxlARGB4444;

typedef union {
	pxlRGB565 rgb565;
	pxlARGB1555 argb1555;
	pxlARGB4444 argb4444;
} pxlColor16;

typedef union {
	pxlRGB888 rgb888;
} pxlColor24;

typedef union {
	pxlARGB8888 argb;
	pxlRGBA8888 rgba;
} pxlColor32;

static inline float pxlSatF(float val) {
	return MIN(MAX(val, 0.0f), 1.0f);
}

static inline unsigned int pxlReduceRnd(unsigned int val, unsigned int rshift) {
	unsigned rndup = rshift > 1 ? (1 << rshift)-1 : 0;
	rndup >>= 1;
	rndup = 0;
	return MIN(val + rndup, 255) >> rshift;
}
static inline unsigned int pxlExpand(unsigned int val, unsigned int srcwidth) {
	assert(srcwidth == 1 || srcwidth >= 4);
	assert(srcwidth <= 8);
	if (srcwidth == 1)
		return val ? 0xff : 0;
	//~ return val << (8 - srcwidth);
	//~ if (srcwidth == 1)
		//~ return val ? 0xff : 0;
	//~ else
	unsigned v = (val << (8 - srcwidth));
	return v | (v >> srcwidth);
}

static inline float pxlU8toF(unsigned val) {
	//Maps 0-255 to [0.0f, 1.0f]
	return MIN((int)val, 255) / 255.0f;
}
static inline float pxlU8BtoF(unsigned val) {
	//Maps 1-255 to [-1.0f, 1.0f]
	//128 maps to 0.0
	return (MIN((int)val, 255) - 128) / 127.0f;
}
static inline unsigned pxlFtoU8B(float val) {
	//Maps [-1.0f, 1.0f] to [1, 255]
	//0.0f maps to 128
	return (CLAMP(-1, val, 1) * 127.0f) + 128.0f;
}


static inline unsigned pxlFloattoSpherical(float fx, float fy, float fz) {
	v3f norm = v3Set(fx,fy,fz);
	
	norm = v3NormalizeS(norm);
	
	float azimuth = 0xff;
	azimuth = atan2(norm.y, norm.x);
	if (azimuth < 0) azimuth += 2*M_PI;
	azimuth = CLAMP(0, azimuth, 255);
	
	float altitude = CLAMP(0, acosf(norm.z), M_PI_2);
	
	const float rnd = 0.5;
	int fixed_azimuth = (uint8_t)(azimuth / (2*M_PI) * 255 + rnd);
	int fixed_altitude = (uint8_t)(altitude / M_PI_2 * 255 + rnd) ^ 0xff;
	
	fixed_altitude &= 0xff;
	fixed_azimuth &= 0xff;
	
	return (fixed_altitude << 8) | fixed_azimuth;

}

static inline unsigned pxlRGBtoSpherical(unsigned x, unsigned y, unsigned z) {
	float fx = pxlU8BtoF(x);
	float fy = pxlU8BtoF(y);
	float fz = pxlU8BtoF(z);
	
	return pxlFloattoSpherical(fx, fy, fz);
}

static inline unsigned pxlRGBtoSphericalTC(unsigned x, unsigned y, unsigned z) {
	//Converts normal like texconv
	v3f v;
	v.x = (x/255.0f) * 2.0f - 1.0f;
	v.y = (y/255.0f) * 2.0f - 1.0f;
	v.z = (z/255.0f);
	
	float radius = v3Length(v);
	float polar = acos(v.z / radius);
	float azimuth = atan2(v.y, v.x);
	
	polar = M_PI_2 - polar;
	polar = (polar / M_PI_2) * 255.0f;
	int S = CLAMP(0, polar, 255);
	
	if (azimuth < 0) azimuth += 2*M_PI;
	azimuth = (azimuth / (2*M_PI)) * 255.0f;
	int R = CLAMP(0, azimuth, 255);
	
	return (S << 8) | R;
}

static inline pxlABGR8888 pxlSphericaltoABGR8888(unsigned norm) {
	float azimuth = (norm & 0xff) / 256.0f * (2*M_PI);
	float altitude = (((norm >> 8)& 0xff) ^ 0xff) / 255.0f * M_PI_2;
	
	
	pxlABGR8888 pxl;
	pxl.r = pxlFtoU8B(sinf(altitude) * cosf(azimuth));
	pxl.g = pxlFtoU8B(sinf(altitude) * sinf(azimuth));
	pxl.b = pxlFtoU8B(cosf(altitude));
	pxl.a = 255;
	
	return pxl;
}

static inline pxlABGR8888 pxlSphericaltoABGR8888TC(unsigned norm) {
	//Converts normal like texconv
	float S = (1.0 - ((norm >> 8) / 255.0)) * M_PI_2;
	float R = ((norm & 0xFF) / 255.0) * 2*M_PI;
	if (R > M_PI) R -= 2*M_PI;
	
	pxlABGR8888 pxl;
	pxl.r = (sinf(S) * cosf(R) + 1.0f) * 0.5f * 255.0f;
	pxl.g = (sinf(S) * sinf(R) + 1.0f) * 0.5f * 255.0f;
	pxl.b = (cosf(S) + 1.0f) * 0.5f * 255.0f;
	pxl.a = 255;
	
	return pxl;
}

static inline pxlARGB4444 pxlSetARGB4444(float r, float g, float b, float a) {
	pxlARGB4444 ret;
	ret.a = CLAMP(0,a*15, 15);
	ret.r = CLAMP(0,r*15, 15);
	ret.g = CLAMP(0,g*15, 15);
	ret.b = CLAMP(0,b*15, 15);
	//~ ret.r = pxlSatF(r)*15;
	//~ ret.g = pxlSatF(g)*15;
	//~ ret.b = pxlSatF(b)*15;
	return ret;
}
static inline pxlARGB1555 pxlSetARGB1555(float r, float g, float b, float a) {
	pxlARGB1555 ret;
	ret.a = pxlSatF(a+0.5);
	ret.r = pxlSatF(r)*31;
	ret.g = pxlSatF(g)*31;
	ret.b = pxlSatF(b)*31;
	return ret;
}
static inline pxlRGB565 pxlSetRGB565(float r, float g, float b) {
	pxlRGB565 ret;
	ret.r = pxlSatF(r)*31;
	ret.g = pxlSatF(g)*63;
	ret.b = pxlSatF(b)*31;
	return ret;
}
static inline pxlRGBA8888 pxlSetRGBA8888(float r, float g, float b, float a) {
	pxlRGBA8888 ret;
	ret.r = pxlSatF(r)*255;
	ret.g = pxlSatF(g)*255;
	ret.b = pxlSatF(b)*255;
	ret.a = pxlSatF(a)*255;
	return ret;
}
static inline pxlABGR8888 pxlSetABGR8888(float r, float g, float b, float a) {
	pxlABGR8888 ret;
	ret.r = pxlSatF(r)*255;
	ret.g = pxlSatF(g)*255;
	ret.b = pxlSatF(b)*255;
	ret.a = pxlSatF(a)*255;
	return ret;
}

#define PXL_ADD_SATURATE_8888(format) \
static inline format pxlAddSaturate ## format (format l, format r) { \
	format ret; \
	ret.r = CLAMP(0, l.r + r.r, 255); \
	ret.g = CLAMP(0, l.r + r.r, 255); \
	ret.b = CLAMP(0, l.r + r.r, 255); \
	ret.a = CLAMP(0, l.r + r.r, 255); \
	return ret; \
}

PXL_ADD_SATURATE_8888(pxlRGBA8888)
PXL_ADD_SATURATE_8888(pxlARGB8888)
PXL_ADD_SATURATE_8888(pxlABGR8888)

static inline pxlRGBA32 pxlSubRGBA32andABGR888(pxlRGBA32 l, pxlABGR8888 r) {
	pxlRGBA32 ret;
	ret.r = l.r - r.r;
	ret.b = l.b - r.b;
	ret.g = l.g - r.g;
	ret.a = l.a - r.a;
	return ret;
}
static inline pxlRGBA32 pxlAddRGBA32(pxlRGBA32 l, pxlRGBA32 r) {
	pxlRGBA32 ret;
	ret.r = l.r + r.r;
	ret.b = l.b + r.b;
	ret.g = l.g + r.g;
	ret.a = l.a + r.a;
	return ret;
}
static inline pxlRGBA32 pxlSubRGBA32(pxlRGBA32 l, pxlRGBA32 r) {
	pxlRGBA32 ret;
	ret.r = l.r - r.r;
	ret.b = l.b - r.b;
	ret.g = l.g - r.g;
	ret.a = l.a - r.a;
	return ret;
}
static inline pxlRGBA32 pxlMulRGBA32Float(pxlRGBA32 l, float r) {
	pxlRGBA32 ret;
	ret.r = l.r * r;
	ret.r = l.g * r;
	ret.r = l.b * r;
	ret.r = l.a * r;
	return ret;
}

static inline pxlRGB565 pxlConvertRGBA8888toRGB565(pxlRGBA8888 color) {
	pxlRGB565 ret;
	ret.r = pxlReduceRnd(color.r, 3);
	ret.g = pxlReduceRnd(color.g, 2);
	ret.b = pxlReduceRnd(color.b, 3);
	return ret;
}

static inline pxlRGB565 pxlConvertARGB8888toRGB565(pxlARGB8888 color) {
	pxlRGB565 ret;
	ret.r = pxlReduceRnd(color.r, 3);
	ret.g = pxlReduceRnd(color.g, 2);
	ret.b = pxlReduceRnd(color.b, 3);
	return ret;
}
static inline pxlRGB565 pxlConvertABGR8888toRGB565(pxlABGR8888 color) {
	pxlRGB565 ret;
	ret.r = pxlReduceRnd(color.r, 3);
	ret.g = pxlReduceRnd(color.g, 2);
	ret.b = pxlReduceRnd(color.b, 3);
	return ret;
}
static inline pxlRGB565 pxlConvertRGBA32toRGB565(pxlRGBA32 color) {
	pxlRGB565 ret;
	ret.r = pxlReduceRnd(CLAMP(0,color.r,255), 3);
	ret.g = pxlReduceRnd(CLAMP(0,color.g,255), 2);
	ret.b = pxlReduceRnd(CLAMP(0,color.b,255), 3);
	return ret;
}

#define pxlConvertToRGB565(X) _Generic((X), \
		pxlRGBA8888: pxlConvertRGBA8888toRGB565, \
		pxlARGB8888: pxlConvertARGB8888toRGB565, \
	)(X)

static inline pxlARGB4444 pxlConvertRGBA8888toARGB4444(pxlRGBA8888 color) {
	pxlARGB4444 ret;
	ret.a = pxlReduceRnd(color.a, 4);
	ret.r = pxlReduceRnd(color.r, 4);
	ret.g = pxlReduceRnd(color.g, 4);
	ret.b = pxlReduceRnd(color.b, 4);
	return ret;
}
static inline pxlARGB4444 pxlConvertARGB8888toARGB4444(pxlARGB8888 color) {
	pxlARGB4444 ret;
	ret.a = pxlReduceRnd(color.a, 4);
	ret.r = pxlReduceRnd(color.r, 4);
	ret.g = pxlReduceRnd(color.g, 4);
	ret.b = pxlReduceRnd(color.b, 4);
	return ret;
}
static inline pxlARGB4444 pxlConvertABGR8888toARGB4444(pxlABGR8888 color) {
	pxlARGB4444 ret;
	ret.a = pxlReduceRnd(color.a, 4);
	ret.r = pxlReduceRnd(color.r, 4);
	ret.g = pxlReduceRnd(color.g, 4);
	ret.b = pxlReduceRnd(color.b, 4);
	return ret;
}
static inline pxlARGB4444 pxlConvertRGBA32toARGB4444(pxlRGBA32 color) {
	pxlARGB4444 ret;
	ret.r = pxlReduceRnd(CLAMP(0,color.r,255), 4);
	ret.g = pxlReduceRnd(CLAMP(0,color.g,255), 4);
	ret.b = pxlReduceRnd(CLAMP(0,color.b,255), 4);
	ret.a = pxlReduceRnd(CLAMP(0,color.a,255), 4);
	return ret;
}
#define pxlConvertToARGB4444(X) _Generic((X), \
		pxlRGBA8888: pxlConvertRGBA8888toARGB4444, \
		pxlARGB8888: pxlConvertARGB8888toARGB4444, \
	)(X)

static inline pxlARGB1555 pxlConvertRGBA8888toARGB1555(pxlRGBA8888 color) {
	pxlARGB1555 ret;
	ret.a = pxlReduceRnd(color.a, 7);
	ret.r = pxlReduceRnd(color.r, 3);
	ret.g = pxlReduceRnd(color.g, 3);
	ret.b = pxlReduceRnd(color.b, 3);
	return ret;
}
static inline pxlARGB1555 pxlConvertARGB8888toARGB1555(pxlARGB8888 color) {
	pxlARGB1555 ret;
	ret.a = pxlReduceRnd(color.a, 7);
	ret.r = pxlReduceRnd(color.r, 3);
	ret.g = pxlReduceRnd(color.g, 3);
	ret.b = pxlReduceRnd(color.b, 3);
	return ret;
}
static inline pxlARGB1555 pxlConvertABGR8888toARGB1555(pxlABGR8888 color) {
	pxlARGB1555 ret;
	ret.a = pxlReduceRnd(color.a, 7);
	ret.r = pxlReduceRnd(color.r, 3);
	ret.g = pxlReduceRnd(color.g, 3);
	ret.b = pxlReduceRnd(color.b, 3);
	return ret;
}
#define pxlConvertToARGB1555(X) _Generic((X), \
		pxlRGBA8888: pxlConvertRGBA8888toARGB1555, \
		pxlARGB8888: pxlConvertARGB8888toARGB1555, \
	)(X)

static inline pxlRGBA8888 pxlConvertRGB565toRGBA8888(pxlRGB565 color) {
	pxlRGBA8888 ret;
	ret.r = pxlExpand(color.r, 5);
	ret.g = pxlExpand(color.g, 6);
	ret.b = pxlExpand(color.b, 5);
	ret.a = 0xff;
	return ret;
}
static inline pxlABGR8888 pxlConvertRGB565toABGR8888(pxlRGB565 color) {
	pxlABGR8888 ret;
	ret.r = pxlExpand(color.r, 5);
	ret.g = pxlExpand(color.g, 6);
	ret.b = pxlExpand(color.b, 5);
	ret.a = 0xff;
	return ret;
}
static inline pxlRGBA8888 pxlConvertARGB4444toRGBA8888(pxlARGB4444 color) {
	pxlRGBA8888 ret;
	ret.r = pxlExpand(color.r, 4);
	ret.g = pxlExpand(color.g, 4);
	ret.b = pxlExpand(color.b, 4);
	ret.a = pxlExpand(color.a, 4);
	return ret;
}
static inline pxlABGR8888 pxlConvertARGB4444toABGR8888(pxlARGB4444 color) {
	pxlABGR8888 ret;
	ret.r = pxlExpand(color.r, 4);
	ret.g = pxlExpand(color.g, 4);
	ret.b = pxlExpand(color.b, 4);
	ret.a = pxlExpand(color.a, 4);
	return ret;
}
static inline pxlRGBA8888 pxlConvertARGB1555toRGBA8888(pxlARGB1555 color) {
	pxlRGBA8888 ret;
	ret.r = pxlExpand(color.r, 5);
	ret.g = pxlExpand(color.g, 5);
	ret.b = pxlExpand(color.b, 5);
	ret.a = pxlExpand(color.a, 1);
	return ret;
}
static inline pxlABGR8888 pxlConvertARGB1555toABGR8888(pxlARGB1555 color) {
	pxlABGR8888 ret;
	ret.r = pxlExpand(color.r, 5);
	ret.g = pxlExpand(color.g, 5);
	ret.b = pxlExpand(color.b, 5);
	ret.a = pxlExpand(color.a, 1);
	return ret;
}
#define pxlConvertToRGBA8888(X) _Generic((X), \
		  pxlRGB565: pxlConvertRGB565toRGBA8888, \
		pxlARGB4444: pxlConvertARGB4444toRGBA8888, \
		pxlARGB1555: pxlConvertARGB1555toRGBA8888, \
	)(X)

static inline pxlARGB8888 pxlConvertRGB565toARGB8888(pxlRGB565 color) {
	pxlARGB8888 ret;
	ret.r = pxlExpand(color.r, 5);
	ret.g = pxlExpand(color.g, 6);
	ret.b = pxlExpand(color.b, 5);
	ret.a = 0xff;
	return ret;
}
static inline pxlARGB8888 pxlConvertARGB4444toARGB8888(pxlARGB4444 color) {
	pxlARGB8888 ret;
	ret.r = pxlExpand(color.r, 4);
	ret.g = pxlExpand(color.g, 4);
	ret.b = pxlExpand(color.b, 4);
	ret.a = pxlExpand(color.a, 4);
	return ret;
}
static inline pxlARGB8888 pxlConvertARGB1555toARGB8888(pxlARGB1555 color) {
	pxlARGB8888 ret;
	ret.r = pxlExpand(color.r, 5);
	ret.g = pxlExpand(color.g, 5);
	ret.b = pxlExpand(color.b, 5);
	ret.a = pxlExpand(color.a, 1);
	return ret;
}
#define pxlConvertToARGB8888(X) _Generic((X), \
		  pxlRGB565: pxlConvertRGB565toARGB8888, \
		pxlARGB4444: pxlConvertARGB4444toARGB8888, \
		pxlARGB1555: pxlConvertARGB1555toARGB8888, \
	)(X)

static inline pxlRGBA32 pxlConvertABGR8888toRGBA32(pxlABGR8888 color) {
	pxlRGBA32 ret;
	ret.r = color.r;
	ret.g = color.g;
	ret.b = color.b;
	ret.a = color.a;
	return ret;
}
static inline pxlARGB8888 pxlConvertABGR8888toARGB8888(pxlABGR8888 color) {
	pxlARGB8888 ret;
	ret.r = color.r;
	ret.g = color.g;
	ret.b = color.b;
	ret.a = color.a;
	return ret;
}
static inline pxlABGR8888 pxlConvertARGB8888toABGR8888(pxlARGB8888 color) {
	pxlABGR8888 ret;
	ret.r = color.r;
	ret.g = color.g;
	ret.b = color.b;
	ret.a = color.a;
	return ret;
}
static inline pxlRGBA32 pxlConvertRGB565toRGBA32(pxlRGB565 color) {
	pxlRGBA32 ret;
	ret.r = pxlExpand(color.r, 5);
	ret.g = pxlExpand(color.g, 6);
	ret.b = pxlExpand(color.b, 5);
	ret.a = 0xff;
	return ret;
}
static inline pxlRGBA32 pxlConvertARGB4444toRGBA32(pxlARGB4444 color) {
	pxlRGBA32 ret;
	ret.r = pxlExpand(color.r, 4);
	ret.g = pxlExpand(color.g, 4);
	ret.b = pxlExpand(color.b, 4);
	ret.a = pxlExpand(color.a, 4);
	return ret;
}

#define PXL_COLOR_WEIGHTS	v4Set(.3,.59,.11, .7)
static inline unsigned pxlFindClosestColor(const pxlABGR8888 src, const pxlABGR8888 *pal, size_t palsize) {
	float bestdist = 1e100;
	unsigned best = 0;
	v4f srcf = v4Mul(v4Set(src.r, src.g, src.b, src.a), PXL_COLOR_WEIGHTS);
	
	for(unsigned i = 0; i < palsize; i++) {
		pxlABGR8888 c = pal[i];
		v4f colorf = v4Mul(v4Set(c.r, c.g, c.b, c.a), PXL_COLOR_WEIGHTS);
		
		float dist = v4SqrDistance(colorf, srcf);
		if (dist < bestdist) {
			bestdist = dist;
			best = i;
		}
	}
	
	return best;
}
