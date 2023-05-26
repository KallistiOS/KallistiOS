/* KallistiOS ##version##

   bspline.h
   Copyright (C) 2000 Megan Potter

*/

#ifndef __KOS_BSPLINE_H
#define __KOS_BSPLINE_H

/** \file   kos/bspline.h
    \brief  B-Spline curve support.

    This module provides utility functions to generate b-spline curves in your
    program. It is used by passing in a set of control points to
    bspline_coeff(), and then querying for individual points using
    bspline_get_point().

    Note that this module is NOT thread-safe.

    \author Megan Potter
*/

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <dc/vector.h>

/** \brief  Calculate and set b-spline coefficients.

    This function performs the initial setup work of calculating the
    coefficients needed to generate a b-spline curve for the specified set of
    points. The calculation is based on a total of 4 points: one previous point,
    the current point, and two points that occur after the current point.

    The current point should be at pnt[0], the previous at pnt[-1], and the
    future points should be at pnt[1], and pnt[2]. I repeat: pnt[-1] must be a
    valid point for this to work properly.

    \param  pnt         The array of points used to calculate the b-spline
                        coefficients.
*/
void bspline_coeff(const point_t *pnt);

/** \brief  Generate the next point for the current set of coefficients.

    Given a 't' (between 0.0f and 1.0f) this will generate the next point value
    for the current set of coefficients.

    \param  t           The "t" value for the b-spline generation function.
    \param  p           Storage for the generated point.
*/
void bspline_get_point(float t, point_t *p);

__END_DECLS

#endif  /* __KOS_BSPLINE_H */
