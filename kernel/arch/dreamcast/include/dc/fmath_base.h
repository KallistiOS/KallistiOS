/* KallistiOS ##version##

   dc/fmath_base.h
   Copyright (C) 2001 Andrew Kieschnick
   Copyright (C) 2014 Josh Pearson

*/

/**
    \file    dc/fmath_base.h
    \brief   Base definitions for the DC's special math instructions
    \ingroup math_intrinsics

    \author Andrew Kieschnick
    \author Josh Pearson
*/

#ifndef __DC_FMATH_BASE_H
#define __DC_FMATH_BASE_H

#include <arch/args.h>

#include <sys/cdefs.h>
__BEGIN_DECLS


/** \addtogroup math_intrinsics 
    @{
*/

/** \brief PI constant (if you don't want full math.h) */
#define F_PI 3.1415926f

/** \cond */
#define __fsin(x) __builtin_sinf(x)
#define __fcos(x) __builtin_cosf(x)
#define __ftan(x) __builtin_tanf(x)
#define __fisin(x) __builtin_sinf((float)(x) / 10430.37835f);
#define __ficos(x) __builtin_cosf((float)(x) / 10430.37835f);
#define __fitan(x) __builtin_tanf((float)(x) / 10430.37835f);

#define __fsincosr(r, s, c) \
    ({  float __r = (r) / 10430.37835f; \
        s = __fsin(__r); \
        c = __fcos(__r); \
    })

#define __fsincos(r, s, c) \
    __fsincosr((r) * 182.04444443f, s, c)

#define __fsqrt(x) __builtin_sqrtf(x)
#define __frsqrt(x) (1.0f / __builtin_sqrtf(x))

/* Floating point inner product (dot product) */
#define __fipr(x, y, z, w, a, b, c, d) ({ \
        register float __x __asm__(KOS_FPARG(0)) = (x); \
        register float __y __asm__(KOS_FPARG(1)) = (y); \
        register float __z __asm__(KOS_FPARG(2)) = (z); \
        register float __w __asm__(KOS_FPARG(3)) = (w); \
        register float __a __asm__(KOS_FPARG(4)) = (a); \
        register float __b __asm__(KOS_FPARG(5)) = (b); \
        register float __c __asm__(KOS_FPARG(6)) = (c); \
        register float __d __asm__(KOS_FPARG(7)) = (d); \
        __asm__ __volatile__( \
                              "fipr	fv8,fv4" \
                              : "+f" (KOS_SH4_SINGLE_ONLY ? __w : __z) \
                              : "f" (__x), "f" (__y), "f" (__z), "f" (__w), \
                              "f" (__a), "f" (__b), "f" (__c), "f" (__d) \
                            ); \
        KOS_SH4_SINGLE_ONLY ? __w : __z; })

/* Floating point inner product w/self (square of vector magnitude) */
#define __fipr_magnitude_sqr(x, y, z, w) ({ \
        register float __x __asm__(KOS_FPARG(0)) = (x); \
        register float __y __asm__(KOS_FPARG(1)) = (y); \
        register float __z __asm__(KOS_FPARG(2)) = (z); \
        register float __w __asm__(KOS_FPARG(3)) = (w); \
        __asm__ __volatile__( \
                              "fipr	fv4,fv4" \
                              : "+f" (KOS_SH4_SINGLE_ONLY ? __w : __z) \
                              : "f" (__x), "f" (__y), "f" (__z), "f" (__w) \
                            ); \
        KOS_SH4_SINGLE_ONLY ? __w : __z; })

/** \endcond */

/** @} */

__END_DECLS

#endif /* !__DC_FMATH_BASE_H */
