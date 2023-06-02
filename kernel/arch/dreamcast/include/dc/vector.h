/* KallistiOS ##version##

   dc/vector.h
   Copyright (C) 2002 Megan Potter

*/

#ifndef __DC_VECTOR_H
#define __DC_VECTOR_H

/** \file   dc/vector.h
    \brief  Primitive matrix, vector, and point types.

    This file provides a few primivite data types that are useful for 3D
    graphics.

    \author Megan Potter
*/

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \brief  Basic 4x4 matrix type.
    \headerfile dc/vector.h
*/
typedef float matrix_t[4][4];

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

__END_DECLS

#endif  /* __DC_VECTOR_H */

