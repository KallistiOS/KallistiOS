/* KallistiOS ##version##

   kos/vector.h
   Copyright (C) 2002 Megan Potter

*/

#ifndef __KOS_VECTOR_H
#define __KOS_VECTOR_H

/** \file   kos/vector.h
    \brief  Primitive matrix, vector, and point types.

    This file provides a few primivite data types that are useful for 3D
    graphics. These are used by the code in kos/bspline.h and can be useful
    elsewhere, as well.

    \author Megan Potter
*/

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/** \brief  Basic 4x4 matrix type.
    \headerfile kos/vector.h
*/
typedef float matrix_t[4][4];

/** \brief  4-part vector type.
    \headerfile kos/vector.h
*/
typedef struct vectorstr {
    float x, y, z, w;
} vector_t;

/** \brief  4-part point type (alias to the vector_t type).
    \headerfile kos/vector.h
*/
typedef vector_t point_t;

__END_DECLS

#endif  /* __KOS_VECTOR_H */

