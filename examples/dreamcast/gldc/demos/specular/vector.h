/* 
   KallistiOS 2.0.0

   vector.h
   (C) 2013 Josh Pearson
*/

#ifndef VECTOR_H
#define VECTOR_H

#include <GL/gl.h>

typedef float vector3f[3];
typedef float vector4f[4];

#define DEG2RAD (F_PI / 180.0f)
#define RAD2DEG (180.0f / F_PI)
#define RADIAN 0.0174532925   // Convert Degrees to Radians
#define CIRCLE 6.2831853      // RADIAN * 360.0f

void  VectorShift(vector3f p, vector3f c, float mag);

#endif
