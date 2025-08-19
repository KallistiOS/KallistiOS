/* KallistiOS ##version##

   dc/vector.h
   Copyright (C) 2002 Megan Potter

*/

#ifndef __DC_VECTOR_H
#define __DC_VECTOR_H

/** \file    dc/vector.h
    \brief   Primitive matrix, vector, and point types.
    \ingroup math_matrices

    This file provides a few primivite data types that are useful for 3D
    graphics.

    \author Megan Potter
*/

#include <kos/cdefs.h>
__BEGIN_DECLS

/** \addtogroup math_matrices
    @{
*/

/** \brief  Basic 4x4 matrix type.
    \headerfile dc/vector.h

    \warning
    This type must always be allocated on 8-byte boundaries,
    or else the API operating on it will crash on unaligned
    accesses. Keep this in mind with heap allocation, where
    you must ensure alignment manually.
*/
typedef __attribute__ ((aligned (8))) float matrix_t[4][4];

/** \brief  4-part vector type.
    \headerfile dc/vector.h
*/
typedef struct vectorstr {
    float x, y, z, w;
} vector_t;

/** \brief  4-part point type (alias to the vector_t type).
    \headerfile dc/vector.h
*/
typedef vector_t point_t;

/** @} */

__END_DECLS

#endif  /* __DC_VECTOR_H */

