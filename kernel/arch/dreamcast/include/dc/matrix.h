/* KallistiOS ##version##

   dc/matrix.h
   Copyright (C) 2000 Megan Potter
   Copyright (C) 2013, 2014 Josh "PH3NOM" Pearson
   Copyright (C) 2018 Lawrence Sebald

*/

/** \file    dc/matrix.h
    \brief   Basic matrix operations.
    \ingroup math_matrices

    This file contains various basic matrix math functionality for using the
    SH4's matrix transformation unit. Higher level functionality, like the 3D
    functionality is built off of these operations.

    \see    dc/matrix3d.h

    \author Megan Potter
    \author Josh "PH3NOM" Pearson
*/

#ifndef __DC_MATRIX_H
#define __DC_MATRIX_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <dc/vector.h>

/** \defgroup math_matrices Matrices
    \brief                  SH4-optimized matrix and linear algebra routines
    \ingroup                math
    @{
*/

/** \brief  Copy the internal matrix to a memory one.

    This function stores the current internal matrix to one in memory.

    \warning
    \p out MUST be at least 8-byte aligned!

    \note
    For best performance, 32-byte alignment of \p out is recommended.

    \param  out             A pointer to where to store the matrix (must be at
                            least 8-byte aligned, should be 32-byte aligned).
*/
void mat_store(matrix_t *out);

/** \brief  Copy a memory matrix into the internal one.

    This function loads the internal matrix with the values of one in memory.

    \warning
    \p out MUST be at least 8-byte aligned!

    \note
    For best performance, 32-byte alignment of \p out is recommended.

    \param  src             A pointer to where to load the matrix from (must be
                            at least 8-byte aligned, should be 32-byte aligned).
*/
void mat_load(const matrix_t *src);

/** \brief  Clear the internal matrix to identity.

    This function clears the internal matrix to a standard identity matrix.
*/
void mat_identity(void);

/** \brief  Apply a matrix.

    This function multiplies a matrix in memory onto the internal matrix.

    \warning
    \p src MUST be at least 8-byte aligned!

    \note
    For best performance, 32-byte alignment of \p src is recommended.

    \param  src             A pointer to the matrix to multiply.
*/
void mat_apply(const matrix_t *src);

/** \brief  Multiply a matrix.

    This function multiplies a matrix in memory with the internal matrix
    and stores the result into a new matrix.

    \warning
    \p src and dst MUST be at least 8-byte aligned!

    \note
    For best performance, 32-byte alignment of \p src and \p dst is recommended.

    \param  dst             A pointer to the destination matrix.
    \param  src             A pointer to the matrix to multiply.
*/
void mat_multiply(matrix_t *dst, const matrix_t *src);

/** \brief  Transform vectors by the internal matrix.

    This function transforms zero or more sets of vectors by the current
    internal matrix. Each vector is 3 single-precision floats long.

    \param  invecs          The list of input vectors.
    \param  outvecs         The list of output vectors.
    \param  veccnt          How many vectors are in the list.
    \param  stride          Number of bytes between vectors.
*/
void mat_transform(const vector_t *invecs, vector_t *outvecs,
                   int veccnt, int stride);

/** \brief  Transform vectors by the internal matrix into the store queues.

    This function transforms one or more sets of vertices using the current
    internal matrix directly into the store queues. Each vertex is exactly
    32-bytes long, and the non-xyz data that is with it will be copied over with
    the transformed coordinates. This is perfect, for instance, for transforming
    pvr_vertex_t vertices.

    \note                   sq_lock() must have been called beforehand

    \param  input           The list of input vertices.
    \param  output          The output pointer (SQ address)
    \param  veccnt          The number of vertices to transform.

    \author Jim Ursetto
*/
void mat_transform_sq(void *input, void *output, int veccnt);

/** \brief  Macro to transform a single vertex by the internal matrix.

    This macro is an inline assembly operation to transform a single vertex. It
    works most efficiently if the x value is in fr0, y is in fr1, and z is in
    fr2 before using the macro.

    \param  x               The X coordinate to transform.
    \param  y               The Y coordinate to transform.
    \param  z               The Z coordinate to transform.
*/
#define mat_trans_single(x, y, z) { \
        register float __x __asm__("fr0") = (x); \
        register float __y __asm__("fr1") = (y); \
        register float __z __asm__("fr2") = (z); \
        __asm__ __volatile__( \
                              "fldi1	fr3\n" \
                              "ftrv	xmtrx,fv0\n" \
                              "fldi1	fr2\n" \
                              "fdiv	fr3,fr2\n" \
                              "fmul	fr2,fr0\n" \
                              "fmul	fr2,fr1\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z) \
                              : "0" (__x), "1" (__y), "2" (__z) \
                              : "fr3" ); \
        x = __x; y = __y; z = __z; \
    }

/** \brief  Macro to transform a single vertex by the internal matrix.

    This macro is an inline assembly operation to transform a single vertex. It
    works most efficiently if the x value is in fr0, y is in fr1, z is in
    fr2, and w is in fr3 before using the macro. This macro is similar to
    \ref mat_trans_single(), but this one allows an input to and preserves the
    Z/W value.

    \param  x               The X coordinate to transform.
    \param  y               The Y coordinate to transform.
    \param  z               The Z coordinate to transform.
    \param  w               The W coordinate to transform.
*/
#define mat_trans_single4(x, y, z, w) { \
        register float __x __asm__("fr0") = (x); \
        register float __y __asm__("fr1") = (y); \
        register float __z __asm__("fr2") = (z); \
        register float __w __asm__("fr3") = (w); \
        __asm__ __volatile__( \
                              "ftrv	xmtrx,fv0\n" \
                              "fdiv	fr3,fr0\n" \
                              "fdiv	fr3,fr1\n" \
                              "fdiv	fr3,fr2\n" \
                              "fldi1	fr4\n" \
                              "fdiv	fr3,fr4\n" \
                              "fmov	fr4,fr3\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z), "=f" (__w) \
                              : "0" (__x), "1" (__y), "2" (__z), "3" (__w) \
                              : "fr4" ); \
        x = __x; y = __y; z = __z; w = __w; \
    }

/** \brief  Macro to transform a single vertex by the internal matrix.

    This macro is an inline assembly operation to transform a single vertex. It
    works most efficiently if the x value is in fr0, y is in fr1, and z is in
    fr2 before using the macro. This macro is similar to
    \ref mat_trans_single(), but this one leaves z/w instead of 1/w for the z
    component.

    \param  x               The X coordinate to transform.
    \param  y               The Y coordinate to transform.
    \param  z               The Z coordinate to transform.
*/
#define mat_trans_single3(x, y, z) { \
        register float __x __asm__("fr0") = (x); \
        register float __y __asm__("fr1") = (y); \
        register float __z __asm__("fr2") = (z); \
        __asm__ __volatile__( \
                              "fldi1	fr3\n" \
                              "ftrv	xmtrx,fv0\n" \
                              "fdiv	fr3,fr0\n" \
                              "fdiv	fr3,fr1\n" \
                              "fdiv	fr3,fr2\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z) \
                              : "0" (__x), "1" (__y), "2" (__z) \
                              : "fr3" ); \
        x = __x; y = __y; z = __z; \
    }

/** \brief  Macro to transform a single vertex by the internal matrix with no
            perspective division.

    This macro is an inline assembly operation to transform a single vertex. It
    works most efficiently if the x value is in fr0, y is in fr1, z is in
    fr2, and w is in fr3 before using the macro. This macro is similar to
    \ref mat_trans_single(), but this one does not do any perspective division.

    \param  x               The X coordinate to transform.
    \param  y               The Y coordinate to transform.
    \param  z               The Z coordinate to transform.
    \param  w               The W coordinate to transform.
*/
#define mat_trans_nodiv(x, y, z, w) { \
        register float __x __asm__("fr0") = (x); \
        register float __y __asm__("fr1") = (y); \
        register float __z __asm__("fr2") = (z); \
        register float __w __asm__("fr3") = (w); \
        __asm__ __volatile__( \
                              "ftrv   xmtrx,fv0\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z), "=f" (__w) \
                              : "0" (__x), "1" (__y), "2" (__z), "3" (__w) ); \
        x = __x; y = __y; z = __z; w = __w; \
    }

/** \brief  Macro to transform a single 3d vertex coordinate by the internal
            matrix with no perspective division.

    This macro is an inline assembly operation to transform a 3 float vertex
    coordinate. It works most efficiently if the x value is in fr12, y is in
    fr13, and z is in fr14 before using the macro. This macro is similar to
    \ref mat_trans_nodiv(), but this one sets the W component to 1 for use with
    a 3d vector.

    \param  x               The X coordinate to transform.
    \param  y               The Y coordinate to transform.
    \param  z               The Z coordinate to transform.
*/
#define mat_trans_single3_nodiv(x, y, z) { \
        register float __x __asm__("fr12") = (x); \
        register float __y __asm__("fr13") = (y); \
        register float __z __asm__("fr14") = (z); \
        __asm__ __volatile__( \
                              "fldi1 fr15\n" \
                              "ftrv  xmtrx, fv12\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z) \
                              : "0" (__x), "1" (__y), "2" (__z) \
                              : "fr15" ); \
        x = __x; y = __y; z = __z; \
    }

/** \brief  Macro to transform a single 3d vertex coordinate by the internal
            matrix with perspective division.

    This macro is an inline assembly operation to transform a 3 float vertex
    coordinate. It works most efficiently if the x value is in fr12, y is in
    fr13, and z is in fr14 before using the macro. This macro is similar to
    \ref mat_trans_single(), but this one does not modify the input operands,
    instead storing the transformed vector to the output operands.

    \param  x               The X coordinate to input transform.
    \param  y               The Y coordinate to input transform.
    \param  z               The Z coordinate to input transform.
    \param  x2              The X coordinate to output transform.
    \param  y2              The Y coordinate to output transform.
    \param  z2              The Z coordinate to output transform.
*/
#define mat_trans_single3_nomod(x, y, z, x2, y2, z2) { \
        register float __x __asm__("fr12") = (x); \
        register float __y __asm__("fr13") = (y); \
        register float __z __asm__("fr14") = (z); \
        __asm__ __volatile__( \
                              "fldi1 fr15\n" \
                              "ftrv    xmtrx, fv12\n" \
                              "fldi1 fr14\n" \
                              "fdiv    fr15, fr14\n" \
                              "fmul    fr14, fr12\n" \
                              "fmul    fr14, fr13\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z) \
                              : "0" (__x), "1" (__y), "2" (__z) \
                              : "fr15" ); \
        x2 = __x; y2 = __y; z2 = __z; \
    }

/** \brief  Macro to transform a single 3d vertex coordinate by the internal
            matrix.

    This macro is an inline assembly operation to transform a 3 float vertex
    coordinate. It works most efficiently if the x value is in fr12, y is in
    fr13, and z is in fr14 before using the macro. This macro is similar to
    \ref mat_trans_single3_nodiv(), but this one does not modify the input
    operands, instead storing the transformed vector to the output operands.

    \param  x               The X coordinate to input transform.
    \param  y               The Y coordinate to input transform.
    \param  z               The Z coordinate to input transform.
    \param  x2              The X coordinate to output transform.
    \param  y2              The Y coordinate to output transform.
    \param  z2              The Z coordinate to output transform.
*/
#define mat_trans_single3_nodiv_nomod(x, y, z, x2, y2, z2) { \
        register float __x __asm__("fr12") = (x); \
        register float __y __asm__("fr13") = (y); \
        register float __z __asm__("fr14") = (z); \
        __asm__ __volatile__( \
                              "fldi1 fr15\n" \
                              "ftrv  xmtrx, fv12\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z) \
                              : "0" (__x), "1" (__y), "2" (__z) \
                              : "fr15" ); \
        x2 = __x; y2 = __y; z2 = __z; \
    }

/** \brief  Macro to transform a single 3d vertex coordinate by the internal
            matrix.

    This macro is an inline assembly operation to transform a 3 float vertex
    coordinate. It works most efficiently if the x value is in fr12, y is in
    fr13, and z is in fr14 before using the macro. This macro is similar to
    \ref mat_trans_single3_nodiv(), but this one stores the W component of
    transform for later perspective divide.

    \param  x               The X coordinate to transform.
    \param  y               The Y coordinate to transform.
    \param  z               The Z coordinate to transform.
    \param  w               The W coordinate output of transform.
*/
#define mat_trans_single3_nodivw(x, y, z, w) { \
        register float __x __asm__("fr12") = (x); \
        register float __y __asm__("fr13") = (y); \
        register float __z __asm__("fr14") = (z); \
        register float __w __asm__("fr15") = 1.0f; \
        __asm__ __volatile__( \
                              "ftrv  xmtrx, fv12\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z), "=f" (__w) \
                              : "0" (__x), "1" (__y), "2" (__z), "3" (__w) ); \
        x = __x; y = __y; z = __z; w = __w; \
    }

/** \brief  Macro to transform a single 3d vertex coordinate by the internal
            matrix both with and without perspective division.

    This macro is an inline assembly operation to transform a 3 float vertex
    coordinate. It works most efficiently if the x value is in fr0, y is in fr1,
    and z is in fr2 before using the macro. This macro is similar to
    \ref mat_trans_single(), but this one is used for transforming input vertex
    with and without perspective division.

    \param  x               The X coordinate to transform without perspective
                            divide.
    \param  y               The Y coordinate to transform without perspective
                            divide.
    \param  z               The Z coordinate to transform without perspective
                            divide.
    \param  xd              The X coordinate to output transform with
                            perspective divide.
    \param  yd              The Y coordinate to output transform with
                            perspective divide.
    \param  zd              The Z coordinate to output transform with
                            perspective divide.
*/
#define mat_trans_single3_nodiv_div(x, y, z, xd, yd, zd) { \
        register float __x __asm__("fr0") = (x); \
        register float __y __asm__("fr1") = (y); \
        register float __z __asm__("fr2") = (z); \
        register float __xd __asm__("fr4"); \
        register float __yd __asm__("fr5"); \
        register float __zd __asm__("fr6"); \
        __asm__ __volatile__( \
                              "fldi1 fr3\n" \
                              "ftrv  xmtrx, fv0\n" \
                              "fmov  fr0, fr4\n" \
                              "fmov  fr1, fr5\n" \
                              "fmov  fr3, fr7\n" \
                              "fldi1 fr6\n" \
                              "fdiv    fr7, fr6\n" \
                              "fmul    fr6, fr4\n" \
                              "fmul    fr6, fr5\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z), \
                                "=f" (__xd), "=f" (__yd), "=f" (__zd) \
                              : "0" (__x), "1" (__y), "2" (__z) \
                              : "fr3" ); \
        x = __x; y = __y; z = __z; xd = __xd; yd = __yd; zd = __zd; \
    }

/** \brief  Macro to transform a single vertex normal by the internal matrix.

    This macro is an inline assembly operation to transform a 3 float vertex
    normal. It works most efficiently if the x value is in fr8, y is in fr9,
    and z is in fr10 before using the macro. This macro is similar to
    \ref mat_trans_nodiv(), but this one sets the W component to 0 in order to
    transform a vertex normal, rather than 1 for a vertex position.

    \param  x               The X normal to transform.
    \param  y               The Y normal to transform.
    \param  z               The Z normal to transform.
*/
#define mat_trans_normal3(x, y, z) { \
        register float __x __asm__("fr8") = (x); \
        register float __y __asm__("fr9") = (y); \
        register float __z __asm__("fr10") = (z); \
        __asm__ __volatile__( \
                              "fldi0 fr11\n" \
                              "ftrv  xmtrx, fv8\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z) \
                              : "0" (__x), "1" (__y), "2" (__z) \
                              : "fr11" ); \
        x = __x; y = __y; z = __z; \
    }

/** \brief  Macro to transform a single vertex normal by the internal matrix.

    This macro is an inline assembly operation to transform a 3 float vertex
    normal. It works most efficiently if the x value is in fr8, y is in fr9,
    and z is in fr10 before using the macro. This macro is similar to
    \ref mat_trans_normal3(), but this one does not modify the input operands,
    instead storing the transformed vector to the output operands.

    \param  x               The X normal to input transform.
    \param  y               The Y normal to input transform.
    \param  z               The Z normal to input transform.
    \param  x2              The X normal to output transform.
    \param  y2              The Y normal to output transform.
    \param  z2              The Z normal to output transform.
*/
#define mat_trans_normal3_nomod(x, y, z, x2, y2, z2) { \
        register float __x __asm__("fr8") = (x); \
        register float __y __asm__("fr9") = (y); \
        register float __z __asm__("fr10") = (z); \
        __asm__ __volatile__( \
                              "fldi0 fr11\n" \
                              "ftrv  xmtrx, fv8\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z) \
                              : "0" (__x), "1" (__y), "2" (__z) \
                              : "fr11" ); \
        x2 = __x; y2 = __y; z2 = __z; \
    }

/** @} */

__END_DECLS

#endif  /* !__DC_MATRIX_H */
