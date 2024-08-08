#ifndef NVMATH_H
#define NVMATH_H

#include <stdint.h>
#include <math.h>

#ifdef __CPLUSPLUS
#include <string.h>
extern "C" {
#endif

typedef int32_t nvint;

typedef union v2i v2i;
typedef union v3i v3i;
typedef union v4i v4i;
typedef union v2f v2f;
typedef union v3f v3f;
typedef union v4f v4f;

union v2i {
	struct {
		nvint x, y;
	};
	nvint v[2];
};

union v3i {
	struct {
		nvint x, y, z;
	};
	nvint v[3];
	v2i xy;
};
union v4i {
	struct {
		nvint x, y, z, w;
	};
	nvint v[4];
	v3i xyz;
};

union v2f {
	struct {
		float x, y;
	};
	float v[2];
	#ifdef __CPLUSPLUS
	inline bool operator <(const v2f &b) const { return memcmp(this, &b, sizeof(b)) < 0; }
	#endif
};

union v3f {
	struct {
		float x, y, z;
	};
	float v[3];
	v2f xy;
	#ifdef __CPLUSPLUS
	inline bool operator <(const v3f &b) const { return memcmp(this, &b, sizeof(b)) < 0; }
	#endif
};

union v4f {
	struct {
		float x, y, z, w;
	};
	float v[4];
	v3f xyz;
	v2f xy;
};

typedef v4f vqf;

//~ #define VMATH_USE_XMTRX
#ifdef VMATH_USE_XMTRX
#include "xmtrx.h"
#endif

typedef union {
	struct {
		float e00, e01, e02, e03;
		float e10, e11, e12, e13;
		float e20, e21, e22, e23;
		float e30, e31, e32, e33;
	};
	struct {
			v4f c[4];
	};
	float m[16];
#ifdef VMATH_USE_XMTRX
	xMatrix xm;
#endif
} m4x4f;

/*
	Select how to preform square roots.

	On KOS, sqrt is somehow set up to always to go a library call. Need to look
	into why. You can force GCC built-in sqrt handling (recommended), or use
	inline asm. Built-ins are recommended since the compiler can optimize
	constant values.

	**GCC is getting passed fno-builtin from $KOS_CFLAGS defined in environ_base.sh**

	With the right compile options, GCC can even generate FSRRA instructions.

	Turn these options on for fast builtins:
		-ffast-math -ffp-contract=fast -mfsrra -mfsca
*/
#define NVMATH_TYPE_BUILTIN	(0)
#define NVMATH_TYPE_LIBRARY	(1)
#define NVMATH_TYPE_SH4_ASM	(2)

#define NVMATH_METHOD	NVMATH_TYPE_BUILTIN


#define NVMATH_MIN(a, b) ((a) < (b) ? (a) : (b))
#define NVMATH_MAX(a, b) ((a) > (b) ? (a) : (b))
#define NVMATH_MIN3(a, b, c) NVMATH_MIN(NVMATH_MIN(a, b), c)
#define NVMATH_MAX3(a, b, c) NVMATH_MAX(NVMATH_MAX(a, b), c)

#if NVMATH_METHOD == NVMATH_TYPE_LIBRARY
	#define NVMATH_SQRT(a)	sqrt(a)
	#define NVMATH_RSQRT(a)	(1.0f/sqrt(a))
	#define NVMATH_SIN(rad) sinf(rad)
	#define NVMATH_COS(rad) cosf(rad)
	#define NVMATH_SINCOS(rad, s, c) sincosf(rad, s, c)
	#define NVMATH_ACOS(rad) acosf(rad)
	#define NVMATH_ABS(v) fabsf(v)
	#define NVMATH_ABSI(v) abs(v)
#elif NVMATH_METHOD == NVMATH_TYPE_BUILTIN
	#define NVMATH_SQRT(a)	__builtin_sqrt(a)
	#define NVMATH_RSQRT(a)	(1.0f/__builtin_sqrt(a))
	#define NVMATH_SIN(rad) __builtin_sinf(rad)
	#define NVMATH_COS(rad) __builtin_cosf(rad)
	#define NVMATH_SINCOS(rad, s, c) __builtin_sincosf(rad, s, c)
	#define NVMATH_ACOS(rad) __builtin_acosf(rad)
	#define NVMATH_ABS(v) __builtin_fabsf(v)
	#define NVMATH_ABSI(v) __builtin_abs(v)
#elif NVMATH_METHOD == NVMATH_TYPE_SH4_ASM
	static inline float NVMFsqrt(float a) {
		__asm__( "fsqrt	%0\n\t"
			: "=f" (a)
			: "0" (a)
			: );

		return a;
	}
	static inline float NVMFsrra(float a) {
		__asm__( "fsrra	%0\n\t"
			: "=f" (a)
			: "0" (a)
			: );

		return a;
	}
	#define NVMATH_SQRT(a)	NVMFsqrt(a)
	#define NVMATH_RSQRT(a)	NVMFsrra(a)

	static inline void NVMATH_SINCOS_I(int angle, float *sine, float *cosine)
	{
		register float __s __asm__("fr2");
		register float __c __asm__("fr3");

		asm(    "lds	%2,fpul\n\t"
			"fsca	fpul,dr2\n\t"
			: "=f" (__s), "=f" (__c)
			: "r" (angle)
			: "fpul");

		*sine = __s; *cosine = __c;
	}
	#define NVMATH_SINCOS(rad, s, c) NVMATH_SINCOS_I((rad) * 10430.37835f, s, c)
#else
	#error NVMATH_METHOD not set
#endif

/*
	vqSlerp uses an NVMATH_ASIN call. This normally requires a function call.
	Defineing NVMMATH_APPROX_ASIN_ACOS replaces NVMATH_ASIN and NVMATH_ACOS
	with approximate functions.
*/

#define NVMMATH_APPROX_ASIN_ACOS
#ifdef NVMMATH_APPROX_ASIN_ACOS
	#undef NVMATH_ASIN
	#undef NVMATH_ACOS

	inline float NVMATH_ASIN(float x) {
		const float scale_factor = .5707963268f;
		float x5 = x * x;
		x5 *= x5;
		x5 *= x;
		return x + scale_factor*x5;
	}

	inline float NVMATH_ACOS(float x) {
		return M_PI_2 - NVMATH_ASIN(x);
	}

#endif

#define v2Init(x,y) {{x, y}}
#define v3Init(x,y,z) {{x, y, z}}
#define v4Init(x,y,z,w) {{x, y, z, w }}
#define v2Pass(vec) (vec).x, (vec).y
#define v3Pass(vec) (vec).x, (vec).y, (vec).z
#define v4Pass(vec) (vec).x, (vec).y, (vec).z, (vec).w
//~ #define mat_elem(mat,col,row) mat[row * 4 + col]


////////////////////////////////////
#define NVM_FF4I(name, paramx, paramy, paramz, paramw, opx, opy, opz, opw) \
	static inline v2i v2 ## i ## name(paramx, paramy) { \
		v2i d; opx; opy; return d; } \
	static inline v3i v3 ## i ## name(paramx, paramy, paramz) { \
		v3i d; opx; opy; opz; return d; } \
	static inline v4i v4 ## i ## name(paramx, paramy, paramz, paramw) { \
		v4i d; opx; opy; opz; opw; return d; }

#define NVM_FF4(name, paramx, paramy, paramz, paramw, opx, opy, opz, opw) \
	static inline v2f v2 ## name(paramx, paramy) { \
		v2f d; opx; opy; return d; } \
	static inline v3f v3 ## name(paramx, paramy, paramz) { \
		v3f d; opx; opy; opz; return d; } \
	static inline v4f v4 ## name(paramx, paramy, paramz, paramw) { \
		v4f d; opx; opy; opz; opw; return d; }

#define NVM_FF1I(name, param, opx, opy, opz, opw) \
	static inline v2i v2 ## i ## name(param) { \
		v2i d; opx; opy; return d; } \
	static inline v3i v3 ## i ## name(param) { \
		v3i d; opx; opy; opz; return d; } \
	static inline v4i v4 ## i ## name(param) { \
		v4i d; opx; opy; opz; opw; return d; }

#define NVM_FF1(name, param, opx, opy, opz, opw) \
	static inline v2f v2 ## name(param) { \
		v2f d; opx; opy; return d; } \
	static inline v3f v3 ## name(param) { \
		v3f d; opx; opy; opz; return d; } \
	static inline v4f v4 ## name(param) { \
		v4f d; opx; opy; opz; opw; return d; }

#define NVM_FFU4(name, opx, opy, opz, opw) \
	static inline v2f v2 ## name(v2f v) { \
		v2f d; opx; opy; return d; } \
	static inline v3f v3 ## name(v3f v) { \
		v3f d; opx; opy; opz; return d; } \
	static inline v4f v4 ## name(v4f v) { \
		v4f d; opx; opy; opz; opw; return d; }

#define NVM_FFB4(name, opx, opy, opz, opw) \
	static inline v2i v2 ## i ## name(v2i l, v2i r) { \
		v2i d; opx; opy; return d; } \
	static inline v3i v3 ## i ## name(v3i l, v3i r) { \
		v3i d; opx; opy; opz; return d; } \
	static inline v4i v4 ## i ## name(v4i l, v4i r) { \
		v4i d; opx; opy; opz; opw; return d; } \
	static inline v2f v2 ## name(v2f l, v2f r) { \
		v2f d; opx; opy; return d; } \
	static inline v3f v3 ## name(v3f l, v3f r) { \
		v3f d; opx; opy; opz; return d; } \
	static inline v4f v4 ## name(v4f l, v4f r) { \
		v4f d; opx; opy; opz; opw; return d; }


#define NVM_FFB4MACV(name, opx, opy, opz, opw) \
	static inline v2i v2 ## i ## name(v2i l, v2i r, v2i a) { \
		v2i d; nvint aa = a.x; nvint rr = r.x; opx; aa = a.y; rr = r.y; opy; return d; } \
	static inline v3i v3 ## i ## name(v3i l, v3i r, v3i a) { \
		v3i d; nvint aa = a.x; nvint rr = r.x; opx; aa = a.y; rr = r.y; opy; aa = a.z; rr = r.z; opz; return d; } \
	static inline v4i v4 ## i ## name(v4i l, v4i r, v4i a) { \
		v4i d; nvint aa = a.x; nvint rr = r.x; opx; aa = a.y; rr = r.y; opy; aa = a.z; rr = r.z; opz; aa = a.w; rr = r.w; opw; return d; } \
	static inline v2f v2 ## name(v2f l, v2f r, v2f a) { \
		v2f d; float aa = a.x; float rr = r.x; opx; aa = a.y; rr = r.y; opy; return d; } \
	static inline v3f v3 ## name(v3f l, v3f r, v3f a) { \
		v3f d; float aa = a.x; float rr = r.x; opx; aa = a.y; rr = r.y; opy; aa = a.z; rr = r.z; opz; return d; } \
	static inline v4f v4 ## name(v4f l, v4f r, v4f a) { \
		v4f d; float aa = a.x; float rr = r.x; opx; aa = a.y; rr = r.y; opy; aa = a.z; rr = r.z; opz; aa = a.w; rr = r.w; opw; return d; }

#define NVM_FFB4MACS(name, opx, opy, opz, opw) \
	static inline v2i v2 ## i ## name(v2i l, v2i r, float a) { \
			v2i d; float aa = a; nvint rr = r.x; opx; rr = r.y; opy; return d; } \
		static inline v3i v3 ## i ## name(v3i l, v3i r, float a) { \
			v3i d; float aa = a; nvint rr = r.x; opx; rr = r.y; opy; rr = r.z; opz; return d; } \
		static inline v4i v4 ## i ## name(v4i l, v4i r, float a) { \
			v4i d; float aa = a; nvint rr = r.x; opx; rr = r.y; opy; rr = r.z; opz; rr = r.w; opw; return d; } \
		static inline v2f v2 ## name(v2f l, v2f r, float a) { \
			v2f d; float aa = a; float rr = r.x; opx; rr = r.y; opy; return d; } \
		static inline v3f v3 ## name(v3f l, v3f r, float a) { \
			v3f d; float aa = a; float rr = r.x; opx; rr = r.y; opy; rr = r.z; opz; return d; } \
		static inline v4f v4 ## name(v4f l, v4f r, float a) { \
			v4f d; float aa = a; float rr = r.x; opx; rr = r.y; opy; rr = r.z; opz; rr = r.w; opw; return d; }

#define NVM_FFB4MACSX(name, opx, opy, opz, opw) \
	static inline v2i v2 ## i ## name(v2i l, float r, v2i a) { \
		v2i d; float aa = a.x; nvint rr = r; opx; aa = a.y; opy; return d; } \
		static inline v3i v3 ## i ## name(v3i l, float r, v3i a) { \
			v3i d; float aa = a.x; nvint rr = r; opx; aa = a.y; opy; aa = a.z; opz; return d; } \
		static inline v2f v2 ## name(v2f l, float r, v2f a) { \
			v2f d; float aa = a.x; float rr = r; opx; aa = a.y; opy; return d; } \
		static inline v3f v3 ## name(v3f l, float r, v3f a) { \
			v3f d; float aa = a.x; float rr = r; opx; aa = a.y; opy; aa = a.z; opz; return d; } \
		static inline v4f v4 ## name(v4f l, float r, v4f a) { \
			v4f d; float aa = a.x; float rr = r; opx; aa = a.y; opy; aa = a.z; opz; aa = a.w; opw; return d; }

#define NVM_FFB4MAC(name, opx, opy, opz, opw) \
	NVM_FFB4MACV(name, opx, opy, opz, opw) \
	NVM_FFB4MACSX(name ## V, opx, opy, opz, opw) \
	NVM_FFB4MACS(name ## S, opx, opy, opz, opw)

#define NVM_FFB4_COMB(name, opx, opy, opz, opw) \
	static inline float v2 ## name(v2f l, v2f r) { \
		return opx opy; } \
	static inline float v3 ## name(v3f l, v3f r) { \
		return opx opy opz; } \
	static inline float v4 ## name(v4f l, v4f r) { \
		return opx opy opz opw; }

#define NVM_FFB4_COMB_I(name, opx, opy, opz, opw) \
	static inline float v2 ## name(v2f l, v2f r) { \
		return opx opy; } \
	static inline float v3 ## name(v3f l, v3f r) { \
		return opx opy opz; } \
	static inline float v4 ## name(v4f l, v4f r) { \
		return opx opy opz opw; }

#define NVM_FFB4S(name, opx, opy, opz, opw) \
	static inline v2i v2 ## i ## name(v2i l, nvint r) { \
		v2i d; opx; opy; return d; } \
	static inline v3i v3 ## i ## name(v3i l, nvint r) { \
		v3i d; opx; opy; opz; return d; } \
	static inline v4i v4 ## i ## name(v4i l, nvint r) { \
		v4i d; opx; opy; opz; opw; return d; } \
	static inline v2f v2 ## name(v2f l, float r) { \
		v2f d; opx; opy; return d; } \
	static inline v3f v3 ## name(v3f l, float r) { \
		v3f d; opx; opy; opz; return d; } \
	static inline v4f v4 ## name(v4f l, float r) { \
		v4f d; opx; opy; opz; opw; return d; }

#define NVM_FFB4RS(name, opx, opy, opz, opw) \
	static inline v2i v2 ## i ## name ## RS(nvint l, v2i r) { \
		v2i d; opx; opy; return d; } \
	static inline v3i v3 ## i ## name(nvint l, v3i r) { \
		v3i d; opx; opy; opz; return d; } \
	static inline v2f v2 ## name ## RS(float l, v2f r) { \
		v2f d; opx; opy; return d; } \
	static inline v3f v3 ## name(float l, v3f r) { \
		v3f d; opx; opy; opz; return d; } \
	static inline v4f v4 ## name(float l, v4f r) { \
		v4f d; opx; opy; opz; opw; return d; }

//Binary op, associative
#define NVM_FFB4B(name, op) \
	NVM_FFB4(name, \
		d.x = l.x op r.x, \
		d.y = l.y op r.y, \
		d.z = l.z op r.z, \
		d.w = l.w op r.w) \
	NVM_FFB4S(name ## S, \
		d.x = l.x op r, \
		d.y = l.y op r, \
		d.z = l.z op r, \
		d.w = l.w op r)

//Binary op, not associative
#define NVM_FFB4BR(name, op) \
	NVM_FFB4B(name, op) \
	NVM_FFB4RS(name ## RS, \
		d.x = l op r.x, \
		d.y = l op r.y, \
		d.z = l op r.z, \
		d.w = l op r.w)

//***OPERATIONS***
/*
	v?Set(float, float, float, float)
		Sets vector
	v?SetR(float)
		Sets vector with value Repeated into all dimensions

	v?Get(float*)
		Creates vector from array of floats

	v?Abs(v?f v)
		|v|
	v?Negate(v?f v)
		-v
	v?Recip(v?f v)
		1.0f / v

	v?(Add/Sub/Mul/Div)(v?f l , v?f r)
		l + r
		l - r
		l * r
		l / r
		Adds, subtracts, multiplies or divides two vectors.

	v?(Add/Sub/Mul/Div)S(v?f l, float r)
		l + r
		l - r
		l * r
		l / r
		Adds, subtracts, multiplies or divides a left vector and a right scalar

	v?(Sub/Div)SR(float l, v?f r)
		l - r
		l / r
		Subtracts or divides a left scalar and a right vector

	v?Mac(v?f l, v?f r, v?f accum)
		l * r + accum
	v?MacV(v?f l, float r, v?f accum)
		l * r + accum
	v?MacS(v?f l, v?f r, float accum)
		l * r + accum

	v?Mdc(v?f l, v?f r, v?f accum)
		l * r - accum
	v?MdcV(v?f l, float r, v?f accum)
		l * r - accum
	v?MdcS(v?f l, v?f r, float accum)
		l * r - accum

	v?Nms(v?f l, v?f r, v?f accum)
		accum - l * r
	v?NmsV(v?f l, float r, v?f accum)
		accum - l * r
	v?NmsS(v?f l, v?f r, float accum)
		accum - l * r

	v?Lerp(v?f start, v?f end, v?f weight)
		start + weight * (end - start)
	v?LerpS(v?f start, v?f end, float weight)
		start + weight * (end - start)

	v?Min(v?f a, v?f b)
		min(a, b)
	v?Max(v?f a, v?f b)
		max(a, b)
	v?MaxE(v?f a)
		max(a.x, a.y, a.z, a.w)

	v?Length(v?f v)
		length of v
	v?SqrLength(v?f v)
		squared length of v
	v?Distance(v?f a, v3f b)
		distance between a and b
	v?SqrDistance(v?f a, v3f b)
		square distance between a and b
	v?Normalize(v?f v)
		v with unit length (Will divide by zero if vector is of length 0)
	v?NormalizeS(v?f v)
		v with unit length (returns zero vector if length zero)

	v?Dot(v?f l, v?f r)
		l dot r
	v2Cross(v2f l, v2f r)
		l cross r
	v3Cross(v3f l, v3f r)
		l cross r
	v3Triple(v3f a, v3f b, v3f c)
		a dot (b cross c)
*/

NVM_FF4I(Set, nvint x, nvint y, nvint z, nvint w,
	d.x = x, d.y = y, d.z = z, d.w = w)
NVM_FF4(Set, float x, float y, float z, float w,
	d.x = x, d.y = y, d.z = z, d.w = w)

NVM_FF1I(Get, const nvint *f,
	d.x = f[0], d.y = f[1], d.z = f[2], d.w = f[3])
NVM_FF1(Get, const float *f,
	d.x = f[0], d.y = f[1], d.z = f[2], d.w = f[3])
NVM_FF1I(SetR, int v,
	d.x = v, d.y = v, d.z = v, d.w = v)
NVM_FF1(SetR, float v,
	d.x = v, d.y = v, d.z = v, d.w = v)

static inline v3f v2Extv3(v2f v, float z) {
	return v3Set(v.x, v.y, z);
}
static inline v4f v2Extv4(v2f v, float z, float w) {
	return v4Set(v.x, v.y, z, w);
}
static inline v4f v3Extv4(v3f v, float w) {
	return v4Set(v.x, v.y, v.z, w);
}

#define v2Zero() v2Set(0, 0)
#define v3Zero() v3Set(0, 0, 0)
#define v4Zero() v4Set(0, 0, 0, 0)

NVM_FFU4(Abs,
	d.x = NVMATH_ABS(v.x), d.y = NVMATH_ABS(v.y), d.z = NVMATH_ABS(v.z), d.w = NVMATH_ABS(v.w))
NVM_FFU4(Negate,
	d.x = -v.x, d.y = -v.y, d.z = -v.z, d.w = -v.w)
NVM_FFU4(Recip,
	d.x = 1.0f/v.x, d.y = 1.0f/v.y, d.z = 1.0f/v.z, d.w = 1.0f/v.w)
NVM_FFB4B(Add, +)
NVM_FFB4BR(Sub, -)
NVM_FFB4B(Mul, *)
NVM_FFB4BR(Div, /)
NVM_FFB4_COMB(Dot,
	l.x * r.x, + l.y * r.y, + l.z * r.z, + l.w * r.w)
NVM_FFB4MAC(Mac,
	d.x = l.x * rr + aa,
	d.y = l.y * rr + aa,
	d.z = l.z * rr + aa,
	d.w = l.w * rr + aa)
NVM_FFB4MAC(Mdc,
	d.x = l.x * rr - aa,
	d.y = l.y * rr - aa,
	d.z = l.z * rr - aa,
	d.w = l.w * rr - aa)
NVM_FFB4MAC(Nms,
	d.x = aa - l.x * rr,
	d.y = aa - l.y * rr,
	d.z = aa - l.z * rr,
	d.w = aa - l.w * rr)
NVM_FFB4MAC(Lerp,
	d.x = l.x + aa * (rr - l.x),
	d.y = l.y + aa * (rr - l.y),
	d.z = l.z + aa * (rr - l.z),
	d.w = l.w + aa * (rr - l.w))

NVM_FFB4(Min,
	d.x = l.x < r.x ? l.x : r.x,
	d.y = l.y < r.y ? l.y : r.y,
	d.z = l.z < r.z ? l.z : r.z,
	d.w = l.w < r.w ? l.w : r.w)
NVM_FFB4(Max,
	d.x = l.x > r.x ? l.x : r.x,
	d.y = l.y > r.y ? l.y : r.y,
	d.z = l.z > r.z ? l.z : r.z,
	d.w = l.w > r.w ? l.w : r.w)
NVM_FFB4S(MinS,
	d.x = l.x < r ? l.x : r,
	d.y = l.y < r ? l.y : r,
	d.z = l.z < r ? l.z : r,
	d.w = l.w < r ? l.w : r)
NVM_FFB4S(MaxS,
	d.x = l.x > r ? l.x : r,
	d.y = l.y > r ? l.y : r,
	d.z = l.z > r ? l.z : r,
	d.w = l.w > r ? l.w : r)

static inline float v2SqrLength(v2f v) {
		return v2Dot(v, v);
	}
	static inline float v3SqrLength(v3f v) {
		return v3Dot(v, v);
	}
	static inline float v4SqrLength(v4f v) {
		return v4Dot(v, v);
}

static inline nvint v2iMinE(v2i v) {
	return NVMATH_MIN(v.x, v.y);
}
static inline nvint v3iMinE(v3i v) {
	return NVMATH_MIN(v.x, NVMATH_MIN(v.y, v.z));
}
static inline float v2MinE(v2f v) {
	return NVMATH_MIN(v.x, v.y);
}
static inline float v3MinE(v3f v) {
	return NVMATH_MIN(v.x, NVMATH_MIN(v.y, v.z));
}
static inline float v4MinE(v4f v) {
	return NVMATH_MIN(v.x, NVMATH_MIN(v.y, NVMATH_MIN(v.z, v.w)));
}
static inline nvint v2iMaxE(v2i v) {
	return NVMATH_MAX(v.x, v.y);
}
static inline nvint v3iMaxE(v3i v) {
	return NVMATH_MAX(v.x, NVMATH_MAX(v.y, v.z));
}
static inline float v2MaxE(v2f v) {
	return NVMATH_MAX(v.x, v.y);
}
static inline float v3MaxE(v3f v) {
	return NVMATH_MAX(v.x, NVMATH_MAX(v.y, v.z));
}
static inline float v4MaxE(v4f v) {
	return NVMATH_MAX(v.x, NVMATH_MAX(v.y, NVMATH_MAX(v.z, v.w)));
}

static inline float v2Sum(v2f v) {
	return v2Dot(v,v2SetR(1));
}
static inline float v3Sum(v3f v) {
	return v3Dot(v,v3SetR(1));
}
static inline float v4Sum(v4f v) {
	return v4Dot(v,v4SetR(1));
}


static inline float v2Length(v2f v) {
	return NVMATH_SQRT(v2Dot(v,v));
}
static inline float v3Length(v3f v) {
	return NVMATH_SQRT(v3Dot(v,v));
}
static inline float v4Length(v4f v) {
	return NVMATH_SQRT(v4Dot(v,v));
}
static inline float v2Distance(v2f a, v2f b) {
	v2f v = v2Sub(a, b);
	return NVMATH_SQRT(v2Dot(v,v));
}
static inline float v3Distance(v3f a, v3f b) {
	v3f v = v3Sub(a, b);
	return NVMATH_SQRT(v3Dot(v,v));
}
static inline float v4Distance(v4f a, v4f b) {
	v4f v = v4Sub(a, b);
	return NVMATH_SQRT(v4Dot(v,v));
}
static inline float v2SqrDistance(v2f a, v2f b) {
	v2f v = v2Sub(a, b);
	return v2Dot(v,v);
}
static inline float v3SqrDistance(v3f a, v3f b) {
	v3f v = v3Sub(a, b);
	return v3Dot(v,v);
}
static inline float v4SqrDistance(v4f a, v4f b) {
	v4f v = v4Sub(a, b);
	return v4Dot(v,v);
}

static inline v3f v3Normalize(v3f v) {
	return v3MulS(v, NVMATH_RSQRT(v3Dot(v,v)));
}
static inline v4f v4Normalize(v4f v) {
	return v4MulS(v, NVMATH_RSQRT(v4Dot(v,v)));
}
static inline v2f v2NormalizeS(v2f v) {
	float rs = NVMATH_RSQRT(v2Dot(v,v));
	return rs ? v2MulS(v, rs) : v;
}
static inline v3f v3NormalizeS(v3f v) {
	float rs = NVMATH_RSQRT(v3Dot(v,v));
	return rs ? v3MulS(v, rs) : v;
}

static inline v4f v4Float(v4i v) {
	v4f r = { {v4Pass(v)} };
	return r;
}

static inline v4i v4Int(v4f v) {
	v4i r = { {v4Pass(v)} };
	return r;
}
static inline v4i v4IntRnd(v4f v) {
	v4i r = { {v4Pass(v4AddS(v, 0.5))} };
	return r;
}

static inline float v2Cross(v2f l, v2f r) {
	return l.x*r.y - l.y*r.x;
}
static inline v3f v3Cross(v3f l, v3f r) {
	v3f d;
	d.x = l.y*r.z - l.z*r.y;
	d.y = l.z*r.x - l.x*r.z;
	d.z = l.x*r.y - l.y*r.x;
	return d;
}
static inline float v3Triple(v3f a, v3f b, v3f c) {
	return v3Dot(a, v3Cross(b, c));
}

#ifndef VMATH_USE_XMTRX
	static inline float m44Mul4Row(m4x4f m, v4f v, int row) {
		return v.x * m.c[0].v[row] + v.y * m.c[1].v[row] + v.z * m.c[2].v[row] + v.w * m.c[3].v[row];
	}
	static inline v4f v4MulMat(m4x4f *m, v4f v) {
		v4f r;
		r.x = m44Mul4Row(*m, v, 0); r.y = m44Mul4Row(*m, v, 1); r.z = m44Mul4Row(*m, v, 2); r.w = m44Mul4Row(*m, v, 3);
		return r;
	}
#else
	static inline v4f v4MulMat(m4x4f *m, v4f v) {
		xmtrxLoad((xMatrix*)m);
		register float x __asm__("fr0") = v.x;
		register float y __asm__("fr1") = v.y;
		register float z __asm__("fr2") = v.z;
		register float w __asm__("fr3") = v.w;
		__asm__ __volatile__( 
			"ftrv   xmtrx,fv0\n"
			: "=f" (x), "=f" (y), "=f" (z), "=f" (w)
			: "0" (x), "1" (y), "2" (z), "3" (w) );
		return v4Set(x, y, z, w);
	}

	static inline void m44Mul4x4(m4x4f * restrict l, m4x4f * restrict r, m4x4f * restrict d) {
		xmtrxLoadMultiply((xMatrix*)l, (xMatrix*)r);
		xmtrxStore((xMatrix*)d);
	}

	static inline void m44Identity(m4x4f * m) {
		xmtrxIdentity(d);
		xmtrxStore(d);
	}
#endif




#ifdef __CPLUSPLUS
}
#endif

#endif
