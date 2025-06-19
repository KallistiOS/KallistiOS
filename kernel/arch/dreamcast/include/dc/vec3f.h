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

#include <math.h>

#include <dc/fmath.h>

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

static inline float vec_fipr(vec3f_t vec) {
    return fipr_magnitude_sqr(vec.x, vec.y, vec.z, 0.0f);
}

/** \brief  Function to return the scalar dot product of two 3d vectors.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions, and returns a single-precision
    floating-point value.

    \param  vec1             The first vector.
    \param  vec2             The second vector.
    \return                  The result of the calculation.
*/
static inline float vec_dot(vec3f_t vec1, vec3f_t vec2) {
    return fipr(vec1.x, vec1.y, vec1.z, 0.0f,
                vec2.x, vec2.y, vec2.z, 0.0f);
}

/** \brief  Macro to return scalar Euclidean length of a 3d vector.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions, and returns a single-precision
    floating-point value.

    \param  vec             The vector.
    \return                 The result of the calculation.
*/
static inline float vec_length(vec3f_t vec) {
    return sqrtf(vec_fipr(vec));
}

/** \brief  Function to return the Euclidean distance between two 3d vectors.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions, and returns a single-precision
    floating-point value.

    \param  vec1             The first vector.
    \param  vec2             The second vector.
    \return                  The result of the calculation.
*/
static inline float vec_distance(vec3f_t vec1, vec3f_t vec2) {
    vec3f_t vec = (vec3f_t){ vec2.x - vec1.x, vec2.y - vec1.y, vec2.z - vec1.z };
    return vec_length(vec);
}

/** \brief  Function to return the normalized version of a vector.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions to calculate a vector that is in the same
    direction as the input vector but with a Euclidean length of one.

    \param  vec             The source vector.
    \return                 The normalized vector.
*/
static inline vec3f_t vec_normalize(vec3f_t vec) {
    float factor = 1.0f / vec_length(vec);
    return (vec3f_t){ vec.x * factor, vec.y * factor, vec.z * factor };
}

/** \brief  Function to return the normalized version of a vector minus another
            vector.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions.

    \param  vec1            The first vector.
    \param  vec2            The second vector.
    \return                 The normalized vector.
*/
static inline vec3f_t vec_sub_normalize(vec3f_t vec1, vec3f_t vec2) {
    vec3f_t vec = (vec3f_t){ vec1.x - vec2.x, vec1.y - vec2.y, vec1.z - vec2.z };

    return vec_normalize(vec);
}

/** \brief  Macro to rotate a vector about its origin on the x, y plane.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions.

    \param  vec             The source vector.
    \param  origin          The origin vector.
    \param  angle           The angle (in radians) of rotation.
*/
static inline vec3f_t vec_rotr_xy(vec3f_t vec, vec3f_t origin, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);

    return (vec3f_t) {
        origin.x + (vec.x - origin.x) * c - (vec.y - origin.y) * s,
        origin.y + (vec.x - origin.x) * s + (vec.y - origin.y) * c,
        vec.z,
    };
}

/** \brief  Macro to rotate a vector about its origin on the x, z plane.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions.

    \param  vec             The source vector.
    \param  origin          The origin vector.
    \param  angle           The angle (in radians) of rotation.
*/
static inline vec3f_t vec_rotr_xz(vec3f_t vec, vec3f_t origin, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);

    return (vec3f_t) {
        origin.x + (vec.x - origin.x) * c - (vec.z - origin.z) * s,
        vec.y,
        origin.z + (vec.x - origin.x) * s + (vec.z - origin.z) * c,
    };
}

/** \brief  Macro to rotate a vector about its origin on the y, z plane.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions.

    \param  vec             The source vector.
    \param  origin          The origin vector.
    \param  angle           The angle (in radians) of rotation.
*/
static inline vec3f_t vec_rotr_yz(vec3f_t vec, vec3f_t origin, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);

    return (vec3f_t) {
        vec.x,
        origin.y + (vec.y - origin.y) * c + (vec.z - origin.z) * s,
        origin.z + (vec.y - origin.y) * s + (vec.z - origin.z) * c,
    };
}

/** \brief  Macro to rotate a vector about its origin on the x, y plane.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions.

    \param  vec             The source vector.
    \param  origin          The origin vector.
    \param  angle           The angle (in degrees) of rotation.
*/
static inline vec3f_t vec_rotd_xy(vec3f_t vec, vec3f_t origin, float angle) {
    return vec_rotr_xy(vec, origin, angle * R_DEG / R_RAD);
}

/** \brief  Macro to rotate a vector about its origin on the x, z plane.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions.

    \param  vec             The source vector.
    \param  origin          The origin vector.
    \param  angle           The angle (in degrees) of rotation.
*/
static inline vec3f_t vec_rotd_xz(vec3f_t vec, vec3f_t origin, float angle) {
    return vec_rotr_xz(vec, origin, angle * R_DEG / R_RAD);
}

/** \brief  Macro to rotate a vector about its origin on the y, z plane.

    This macro is an inline assembly operation using the SH4's fast
    (approximate) math instructions.

    \param  vec             The source vector.
    \param  origin          The origin vector.
    \param  angle           The angle (in degrees) of rotation.
*/
static inline vec3f_t vec_rotd_yz(vec3f_t vec, vec3f_t origin, float angle) {
    return vec_rotr_yz(vec, origin, angle * R_DEG / R_RAD);
}

/** \cond */
/* Compatibility macros */
#define vec3f_dot(x1, y1, z1, x2, y2, z2, w) \
    w = vec_dot((vec3f_t){ x1, y1, z1 }, (vec3f_t){ x2, y2, z2 })

#define vec3f_length(x, y, z, w) \
    w = vec_length((vec3f_t){ x, y, z })

#define vec3f_distance(x1, y1, z1, x2, y2, z2, w) \
    w = vec_distance((vec3f_t){ x1, y1, z1 }, (vec3f_t){ x2, y2, z2 })

#define vec3f_normalize(__x, __y, __z) { \
    vec3f_t vec = vec_normalize((vec3f_t){ __x, __y, __z }); \
    __x = vec.x; __y = vec.y; __z = vec.z; \
}

#define vec3f_sub_normalize(x1, y1, z1, x2, y2, z2, x3, y3, z3) { \
    vec3f_t vec = vec_sub_normalize((vec3f_t){ x1, y1, z1 }, (vec3f_t){ x2, y2, z2 }); \
    x3 = vec.x; y3 = vec.y; z3 = vec.z; \
}

#define vec3f_rotr_xy(px, py, pz, cx, cy, cz, r) { \
    vec3f_t vec = vec_rotr_xy((vec3f_t){ px, py, pz }, (vec3f_t){ cx, cy, cz }, r); \
    px = vec.x; py = vec.y; pz = vec.z; \
}

#define vec3f_rotr_xz(px, py, pz, cx, cy, cz, r) { \
    vec3f_t vec = vec_rotr_xz((vec3f_t){ px, py, pz }, (vec3f_t){ cx, cy, cz }, r); \
    px = vec.x; py = vec.y; pz = vec.z; \
}

#define vec3f_rotr_yz(px, py, pz, cx, cy, cz, r) { \
    vec3f_t vec = vec_rotr_yz((vec3f_t){ px, py, pz }, (vec3f_t){ cx, cy, cz }, r); \
    px = vec.x; py = vec.y; pz = vec.z; \
}

#define vec3f_rotd_xy(px, py, pz, cx, cy, cz, r) \
    vec3f_rotr_xy(px, py, pz, cx, cy, cz, (r) * R_DEG / R_RAD)

#define vec3f_rotd_xz(px, py, pz, cx, cy, cz, r) \
    vec3f_rotr_xz(px, py, pz, cx, cy, cz, (r) * R_DEG / R_RAD)

#define vec3f_rotd_yz(px, py, pz, cx, cy, cz, r) \
    vec3f_rotr_yz(px, py, pz, cx, cy, cz, (r) * R_DEG / R_RAD)
/* \endcond */

/** @} */

__END_DECLS

#endif /* !__DC_VEC3F_H */
