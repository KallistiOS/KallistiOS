/* KallistiOS ##version##

   dc/fmath.c
   Copyright (C) 2001 Andrew Kieschnick
*/

#include <arch/types.h>
#include <dc/fmath_base.h>

/* v1 dot v2 (inner product) */
float __pure fipr(float x, float y, float z, float w, float a, float b, float c,
           float d) {
    return __fipr(x, y, z, w, a, b, c, d);
}

/* v1 dot v1 (square of magnitude) */
float __pure fipr_magnitude_sqr(float x, float y, float z, float w) {
    return __fipr_magnitude_sqr(x, y, z, w);
}

/* Returns sin(r), where r is [0..2*PI] */
float __pure fsin(float r) {
    return __fsin(r);
}

/* Returns cos(r), where r is [0..2*PI] */
float __pure fcos(float r) {
    return __fcos(r);
}

/* Returns tan(r), where r is [0..2*PI] */
float __pure ftan(float r) {
    return __ftan(r);
}

/* Returns sin(d), where d is [0..65535] */
float __pure fisin(int d) {
    return __fisin(d);
}

/* Returns cos(d), where d is [0..65535] */
float __pure ficos(int d) {
    return __ficos(d);
}

/* Returns tan(d), where d is [0..65535] */
float __pure fitan(int d) {
    return __fitan(d);
}

/* Returns sqrt(f) */
float __pure fsqrt(float f) {
    return __fsqrt(f);
}

/* Returns 1.0f / sqrt(f) */
float __pure frsqrt(float f) {
    return __frsqrt(f);
}

void fsincos(float f, float *s, float *c) {
    __fsincos(f, *s, *c);
}

void fsincosr(float f, float *s, float *c) {
    __fsincosr(f, *s, *c);
}

uint32 __pure pvr_pack_bump(float h, float t, float q) {
    uint8 hp = (uint8)(h * 255.0f);
    uint8 k1 = ~hp;
    uint8 k2 = (uint8)(hp * __fsin(t));
    uint8 k3 = (uint8)(hp * __fcos(t));
    uint8 qp = (uint8)((q / (2 * F_PI)) * 255.0f);

    return (k1 << 24) | (k2 << 16) | (k3 << 8) | qp;
}
