/* 
   KallistiOS 2.0.0

   texture.h
   (C) 2013 Josh Pearson
*/

#ifndef TEXTURE_H
#define TEXTURE_H

#include <KGL/gl.h>

GLuint glTextureLoadPVR(char *fname, unsigned char isMipMapped, unsigned char glMipMap);

#endif
