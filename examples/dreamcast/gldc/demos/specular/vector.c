/* 
   KallistiOS 2.0.0

   vector.c
   (C) 2013 Josh Pearson

   A set of functions for working with 3D Vectors optimized for the SH4
   Note: Nearly all of these functions have been moved to dc/vec3f.h
*/

#include <kos.h>

#include "vector.h"

void VectorShift(vector3f p, vector3f c, float mag) {
    float d[3] = { (p[0] - c[0]) *mag,
                   (p[1] - c[1]) *mag,
                   (p[2] - c[2]) *mag
                 };
    p[0] += d[0];
    p[1] += d[1];
    p[2] += d[2];
    c[0] += d[0];
    c[1] += d[1];
    c[2] += d[2];
}


