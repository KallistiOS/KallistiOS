/* KallistiOS ##version##

   dc/vec3f.h
   Copyright (C) 2013, 2014 Josh "PH3NOM" Pearson

*/

/** \file    dc/vec3f.h
    \brief   Basic matrix operations.
    \ingroup math_matrices

    This file contains various basic vector math functionality for using the
    SH4's vector instructions. Higher level functionality in KGL is built off
    of these.

    \author Josh "PH3NOM" Pearson
    \see    dc/matrix.h
*/

#ifndef __DC_VEC3F_H
#define __DC_VEC3F_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \addtogroup math_matrices
    @{
*/

/** \brief 3D floating-point vector */
typedef struct vec3f {
    float x, y, z;
} vec3f_t;

/** \cond */
#define R_DEG 182.04444443623349541909523793743f
#define R_RAD 10430.37835f
/* \endcond */

/** \brief  Macro to return the scalar dot product of two 3d vectors.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions, and returns a single-precision
    floating-point value.

    \param  x1               The X coordinate of first vector.
    \param  y1               The Y coordinate of first vector.
    \param  z1               The Z coordinate of first vector.
    \param  x2               The X coordinate of second vector.
    \param  y2               The Y coordinate of second vector.
    \param  z2               The Z coordinate of second vector.
    \param  w                The result of the calculation.
*/
#define vec3f_dot(x1, y1, z1, x2, y2, z2, w) { \
        register float __x __asm__("fr0") = (x1); \
        register float __y __asm__("fr1") = (y1); \
        register float __z __asm__("fr2") = (z1); \
        register float __w __asm__("fr3"); \
        register float __a __asm__("fr4") = (x2); \
        register float __b __asm__("fr5") = (y2); \
        register float __c __asm__("fr6") = (z2); \
        register float __d __asm__("fr7"); \
        __asm__ __volatile__( \
                              "fldi0 fr3\n" \
                              "fldi0 fr7\n" \
                              "fipr    fv4,fv0" \
                              : "+f" (__w) \
                              : "f" (__x), "f" (__y), "f" (__z), "f" (__w), \
                              "f" (__a), "f" (__b), "f" (__c), "f" (__d) \
                            ); \
        w = __w; \
    }

/** \brief  Macro to return scalar Euclidean length of a 3d vector.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions, and returns a single-precision
    floating-point value.

    \param  x               The X coordinate of vector.
    \param  y               The Y coordinate of vector.
    \param  z               The Z coordinate of vector.
    \param  w               The result of the calculation.
*/
#define vec3f_length(x, y, z, w) { \
        register float __x __asm__("fr0") = (x); \
        register float __y __asm__("fr1") = (y); \
        register float __z __asm__("fr2") = (z); \
        register float __w __asm__("fr3"); \
        __asm__ __volatile__( \
                              "fldi0 fr3\n" \
                              "fipr  fv0,fv0\n" \
                              "fsqrt fr3\n" \
                              : "+f" (__w) \
                              : "f" (__x), "f" (__y), "f" (__z), "f" (__w) \
                            ); \
        w = __w; \
    }

/** \brief  Macro to return the Euclidean distance between two 3d vectors.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions, and returns a single-precision
    floating-point value.

    \param  x1               The X coordinate of first vector.
    \param  y1               The Y coordinate of first vector.
    \param  z1               The Z coordinate of first vector.
    \param  x2               The X coordinate of second vector.
    \param  y2               The Y coordinate of second vector.
    \param  z2               The Z coordinate of second vector.
    \param  w                The result of the calculation.
*/
#define vec3f_distance(x1, y1, z1, x2, y2, z2, w) { \
        register float __x  __asm__("fr0") = (x2-x1); \
        register float __y  __asm__("fr1") = (y2-y1); \
        register float __z  __asm__("fr2") = (z2-z1); \
        register float __w  __asm__("fr3"); \
        __asm__ __volatile__( \
                              "fldi0 fr3\n" \
                              "fipr  fv0,fv0\n" \
                              "fsqrt fr3\n" \
                              : "+f" (__w) \
                              : "f" (__x), "f" (__y), "f" (__z), "f" (__w) \
                            ); \
        w = __w; \
    }

/** \brief  Macro to return the normalized version of a vector.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions to calculate a vector that is in the same
    direction as the input vector but with a Euclidean length of one. The input
    vector is modified by the operation as the resulting values.

    \param  x               The X coordinate of vector.
    \param  y               The Y coordinate of vector.
    \param  z               The Z coordinate of vector.
*/
#define vec3f_normalize(x, y, z) { \
        register float __x __asm__("fr0") = x; \
        register float __y __asm__("fr1") = y; \
        register float __z __asm__("fr2") = z; \
        __asm__ __volatile__( \
                              "fldi0 fr3\n" \
                              "fipr  fv0,fv0\n" \
                              "fsrra fr3\n" \
                              "fmul  fr3, fr0\n" \
                              "fmul  fr3, fr1\n" \
                              "fmul  fr3, fr2\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z) \
                              : "0" (__x), "1" (__y), "2" (__z) \
                              : "fr3" ); \
        x = __x; y = __y; z = __z; \
    }

/** \brief  Macro to return the normalized version of a vector minus another
            vector.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions. The return vector is stored into the third
    vertex parameter: x3, y3, and z3.

    \param  x1               The X coordinate of first vector.
    \param  y1               The Y coordinate of first vector.
    \param  z1               The Z coordinate of first vector.
    \param  x2               The X coordinate of second vector.
    \param  y2               The Y coordinate of second vector.
    \param  z2               The Z coordinate of second vector.
    \param  x3               The X coordinate of output vector.
    \param  y3               The Y coordinate of output vector.
    \param  z3               The Z coordinate of output vector.
*/
#define vec3f_sub_normalize(x1, y1, z1, x2, y2, z2, x3, y3, z3) { \
        register float __x __asm__("fr0") = x1 - x2; \
        register float __y __asm__("fr1") = y1 - y2; \
        register float __z __asm__("fr2") = z1 - z2; \
        __asm__ __volatile__( \
                              "fldi0 fr3\n" \
                              "fipr  fv0,fv0\n" \
                              "fsrra fr3\n" \
                              "fmul  fr3, fr0\n" \
                              "fmul  fr3, fr1\n" \
                              "fmul  fr3, fr2\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z) \
                              : "0" (__x), "1" (__y), "2" (__z) \
                              : "fr3" ); \
        x3 = __x; y3 = __y; z3 = __z; \
    }

/** \brief  Macro to rotate a vector about its origin on the x, y plane.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions. The return vector is stored into the first
    vertex parameter: x1, y1, and z1.

    \param  px               The X coordinate of vector to rotate.
    \param  py               The Y coordinate of vector to rotate.
    \param  pz               The Z coordinate of vector to rotate.
    \param  cx               The X coordinate of origin vector.
    \param  cy               The Y coordinate of origin vector.
    \param  cz               The Z coordinate of origin vector.
    \param  r                The angle (in radians) of rotation.
*/
#define vec3f_rotr_xy(px, py, pz, cx, cy, cz, r) { \
        register float __px __asm__("fr0") = px; \
        register float __py __asm__("fr1") = py; \
        register float __cx __asm__("fr4") = cx; \
        register float __cy __asm__("fr5") = cy; \
        register float __r  __asm__("fr6") = r; \
        register float __s __asm__("fr7") = R_RAD; \
        __asm__ __volatile__( \
                              "fmul fr7, fr6\n" \
                              "ftrc fr6, fpul\n" \
                              "fsca fpul, dr6\n" \
                              "fsub fr4, fr0\n" \
                              "fsub fr5, fr1\n" \
                              "fmov fr0, fr2\n" \
                              "fmov fr1, fr3\n" \
                              "fmul fr7, fr0\n" \
                              "fmul fr6, fr1\n" \
                              "fmul fr6, fr2\n" \
                              "fmul fr7, fr3\n" \
                              "fadd fr0, fr4\n" \
                              "fsub fr1, fr4\n" \
                              "fadd fr2, fr5\n" \
                              "fadd fr3, fr5\n" \
                              : "+f" (__cx), "+f" (__cy) \
                              : "f" (__px), "f" (__py), "f" (__r), "f" (__s) ); \
        px = __cx; py = __cy; \
    }

/** \brief  Macro to rotate a vector about its origin on the x, z plane.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions. The return vector is stored into the first
    vertex parameter: x1, y1, and z1.

    \param  px               The X coordinate of vector to rotate.
    \param  py               The Y coordinate of vector to rotate.
    \param  pz               The Z coordinate of vector to rotate.
    \param  cx               The X coordinate of origin vector.
    \param  cy               The Y coordinate of origin vector.
    \param  cz               The Z coordinate of origin vector.
    \param  r                The angle (in radians) of rotation.
*/
#define vec3f_rotr_xz(px, py, pz, cx, cy, cz, r) { \
        register float __px __asm__("fr0") = px; \
        register float __pz __asm__("fr1") = pz; \
        register float __cx __asm__("fr4") = cx; \
        register float __cz __asm__("fr5") = cz; \
        register float __r  __asm__("fr6") = r; \
        register float __s __asm__("fr7") = R_RAD; \
        __asm__ __volatile__( \
                              "fmul fr7, fr6\n" \
                              "ftrc fr6, fpul\n" \
                              "fsca fpul, dr6\n" \
                              "fsub fr4, fr0\n" \
                              "fsub fr5, fr1\n" \
                              "fmov fr0, fr2\n" \
                              "fmov fr1, fr3\n" \
                              "fmul fr7, fr0\n" \
                              "fmul fr6, fr1\n" \
                              "fmul fr6, fr2\n" \
                              "fmul fr7, fr3\n" \
                              "fadd fr0, fr4\n" \
                              "fsub fr1, fr4\n" \
                              "fadd fr2, fr5\n" \
                              "fadd fr3, fr5\n" \
                              : "+f" (__cx), "+f" (__cz) \
                              : "f" (__px), "f" (__pz), "f" (__r), "f" (__s) ); \
        px = __cx; pz = __cz; \
    }

/** \brief  Macro to rotate a vector about its origin on the y, z plane.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions. The return vector is stored into the first
    vertex parameter: x1, y1, and z1.

    \param  px               The X coordinate of vector to rotate.
    \param  py               The Y coordinate of vector to rotate.
    \param  pz               The Z coordinate of vector to rotate.
    \param  cx               The X coordinate of origin vector.
    \param  cy               The Y coordinate of origin vector.
    \param  cz               The Z coordinate of origin vector.
    \param  r                The angle (in radians) of rotation.
*/
#define vec3f_rotr_yz(px, py, pz, cx, cy, cz, r) { \
        register float __py __asm__("fr0") = py; \
        register float __pz __asm__("fr1") = pz; \
        register float __cy __asm__("fr4") = cy; \
        register float __cz __asm__("fr5") = cz; \
        register float __r  __asm__("fr6") = r; \
        register float __s __asm__("fr7") = R_RAD; \
        __asm__ __volatile__( \
                              "fmul fr7, fr6\n" \
                              "ftrc fr6, fpul\n" \
                              "fsca fpul, dr6\n" \
                              "fsub fr4, fr0\n" \
                              "fsub fr5, fr1\n" \
                              "fmov fr0, fr2\n" \
                              "fmov fr1, fr3\n" \
                              "fmul fr7, fr0\n" \
                              "fmul fr6, fr1\n" \
                              "fmul fr6, fr2\n" \
                              "fmul fr7, fr3\n" \
                              "fadd fr0, fr4\n" \
                              "fsub fr1, fr4\n" \
                              "fadd fr2, fr5\n" \
                              "fadd fr3, fr5\n" \
                              : "+f" (__cy), "+f" (__cz) \
                              : "f" (__py), "f" (__pz), "f" (__r), "f" (__s) ); \
        py = __cy; pz = __cz; \
    }

/** \brief  Macro to rotate a vector about its origin on the x, y plane.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions. The return vector is stored into the first
    vertex parameter: x1, y1, and z1.

    \param  px               The X coordinate of vector to rotate.
    \param  py               The Y coordinate of vector to rotate.
    \param  pz               The Z coordinate of vector to rotate.
    \param  cx               The X coordinate of origin vector.
    \param  cy               The Y coordinate of origin vector.
    \param  cz               The Z coordinate of origin vector.
    \param  r                The angle (in degrees) of rotation.
*/
#define vec3f_rotd_xy(px, py, pz, cx, cy, cz, r) { \
        register float __px __asm__("fr0") = px; \
        register float __pz __asm__("fr1") = pz; \
        register float __cx __asm__("fr4") = cx; \
        register float __cz __asm__("fr5") = cz; \
        register float __r  __asm__("fr6") = r; \
        register float __s __asm__("fr7") = R_DEG; \
        __asm__ __volatile__( \
                              "fmul fr7, fr6\n" \
                              "ftrc fr6, fpul\n" \
                              "fsca fpul, dr6\n" \
                              "fsub fr4, fr0\n" \
                              "fsub fr5, fr1\n" \
                              "fmov fr0, fr2\n" \
                              "fmov fr1, fr3\n" \
                              "fmul fr7, fr0\n" \
                              "fmul fr6, fr1\n" \
                              "fmul fr6, fr2\n" \
                              "fmul fr7, fr3\n" \
                              "fadd fr0, fr4\n" \
                              "fsub fr1, fr4\n" \
                              "fadd fr2, fr5\n" \
                              "fadd fr3, fr5\n" \
                              : "+f" (__cx), "+f" (__cz) \
                              : "f" (__px), "f" (__pz), "f" (__r), "f" (__s) ); \
        px = __cx; pz = __cz; \
    }

/** \brief  Macro to rotate a vector about its origin on the x, z plane.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions. The return vector is stored into the first
    vertex parameter: x1, y1, and z1.

    \param  px               The X coordinate of vector to rotate.
    \param  py               The Y coordinate of vector to rotate.
    \param  pz               The Z coordinate of vector to rotate.
    \param  cx               The X coordinate of origin vector.
    \param  cy               The Y coordinate of origin vector.
    \param  cz               The Z coordinate of origin vector.
    \param  r                The angle (in degrees) of rotation.
*/
#define vec3f_rotd_xz(px, py, pz, cx, cy, cz, r) { \
        register float __px __asm__("fr0") = px; \
        register float __pz __asm__("fr1") = pz; \
        register float __cx __asm__("fr4") = cx; \
        register float __cz __asm__("fr5") = cz; \
        register float __r  __asm__("fr6") = r; \
        register float __s __asm__("fr7") = R_DEG; \
        __asm__ __volatile__( \
                              "fmul fr7, fr6\n" \
                              "ftrc fr6, fpul\n" \
                              "fsca fpul, dr6\n" \
                              "fsub fr4, fr0\n" \
                              "fsub fr5, fr1\n" \
                              "fmov fr0, fr2\n" \
                              "fmov fr1, fr3\n" \
                              "fmul fr7, fr0\n" \
                              "fmul fr6, fr1\n" \
                              "fmul fr6, fr2\n" \
                              "fmul fr7, fr3\n" \
                              "fadd fr0, fr4\n" \
                              "fsub fr1, fr4\n" \
                              "fadd fr2, fr5\n" \
                              "fadd fr3, fr5\n" \
                              : "+f" (__cx), "+f" (__cz) \
                              : "f" (__px), "f" (__pz), "f" (__r), "f" (__s) ); \
        px = __cx; pz = __cz; \
    }

/** \brief  Macro to rotate a vector about its origin on the y, z plane.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions. The return vector is stored into the first
    vertex parameter: x1, y1, and z1.

    \param  px               The X coordinate of vector to rotate.
    \param  py               The Y coordinate of vector to rotate.
    \param  pz               The Z coordinate of vector to rotate.
    \param  cx               The X coordinate of origin vector.
    \param  cy               The Y coordinate of origin vector.
    \param  cz               The Z coordinate of origin vector.
    \param  r                The angle (in degrees) of rotation.
*/
#define vec3f_rotd_yz(px, py, pz, cx, cy, cz, r) { \
        register float __py __asm__("fr0") = py; \
        register float __pz __asm__("fr1") = pz; \
        register float __cy __asm__("fr4") = cy; \
        register float __cz __asm__("fr5") = cz; \
        register float __r  __asm__("fr6") = r; \
        register float __s __asm__("fr7") = R_DEG; \
        __asm__ __volatile__( \
                              "fmul fr7, fr6\n" \
                              "ftrc fr6, fpul\n" \
                              "fsca fpul, dr6\n" \
                              "fsub fr4, fr0\n" \
                              "fsub fr5, fr1\n" \
                              "fmov fr0, fr2\n" \
                              "fmov fr1, fr3\n" \
                              "fmul fr7, fr0\n" \
                              "fmul fr6, fr1\n" \
                              "fmul fr6, fr2\n" \
                              "fmul fr7, fr3\n" \
                              "fadd fr0, fr4\n" \
                              "fsub fr1, fr4\n" \
                              "fadd fr2, fr5\n" \
                              "fadd fr3, fr5\n" \
                              : "+f" (__cy), "+f" (__cz) \
                              : "f" (__py), "f" (__pz), "f" (__r), "f" (__s) ); \
        py = __cy; pz = __cz; \
    }

/** @} */

__END_DECLS

#endif /* !__DC_VEC3F_H */
