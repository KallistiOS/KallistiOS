/* KallistiOS ##version##

   matrix3d.h
   (c)2000 Megan Potter and Jordan DeLong

*/

/** \file    dc/matrix3d.h
    \brief   3D matrix operations.
    \ingroup math_matrices

    This file contains various 3D matrix math functionality for using the SH4's
    matrix transformation unit.

    \author Megan Potter
    \author Jordan DeLong
*/

#ifndef __KOS_MATRIX3D_H
#define __KOS_MATRIX3D_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <dc/matrix.h>

/** \addtogroup math_matrices
    @{
*/

/** \brief  Rotate around the X-axis.

    This function sets up a rotation matrix around the X-axis.

    \param  r               The angle to rotate, in radians.
*/
void mat_rotate_x(float r);

/** \brief  Rotate around the Y-axis.

    This function sets up a rotation matrix around the Y-axis.

    \param  r               The angle to rotate, in radians.
*/
void mat_rotate_y(float r);

/** \brief  Rotate around the Z-axis.

    This function sets up a rotation matrix around the Z-axis.

    \param  r               The angle to rotate, in radians.
*/
void mat_rotate_z(float r);

/** \brief  Rotate around all axes.

    This function sets up a rotation matrix around the X-axis, then around the
    Y, then around the Z.

    \param  xr              The angle to rotate around the X-axis, in radians.
    \param  yr              The angle to rotate around the Y-axis, in radians.
    \param  zr              The angle to rotate around the Z-axis, in radians.
*/
void mat_rotate(float xr, float yr, float zr);

/** \brief  Perform a 3D translation.

    This function sets up a translation matrix with the specified parameters.

    \param  x               The amount to translate in X.
    \param  y               The amount to translate in Y.
    \param  z               The amount to translate in Z.
*/
void mat_translate(float x, float y, float z);

/** \brief  Perform a 3D scale operation.

    This function sets up a scaling matrix with the specified parameters.

    \param  x               The ratio to scale in X.
    \param  y               The ratio to scale in Y.
    \param  z               The ratio to scale in Z.
*/
void mat_scale(float x, float y, float z);

/** \brief  Set up a perspective view frustum.

    This function sets up a perspective view frustum for basic 3D usage.

    \param  xcenter         Center of the X direction.
    \param  ycenter         Center of the Y direction.
    \param  cot_fovy_2      1.0 / tan(view_angle / 2).
    \param  znear           Near Z-plane.
    \param  zfar            Far Z-plane.
*/
void mat_perspective(float xcenter, float ycenter, float cot_fovy_2,
                     float znear, float zfar);

/** \brief  Set up a "camera".

    This function acts as the similarly named GL function to set up a "camera"
    by doing rotations/translations.

    \param  eye             The eye coordinate.
    \param  center          The center coordinate.
    \param  up              The up vector.
*/
void mat_lookat(const point_t * eye, const point_t * center, const vector_t * up);

/** @} */

__END_DECLS

#endif  /* __KOS_MATRIX3D_H */


