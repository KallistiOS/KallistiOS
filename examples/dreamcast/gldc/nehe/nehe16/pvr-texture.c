/*
   KallistiOS 2.0.0

   pvr-texture.c
   (c)2014 Josh PH3NOM Pearson

   Load A PVR Texture to the PVR using Open GL
*/
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glkos.h>
#include <GL/glext.h>

#define PVR_HDR_SIZE 0x20
#define MAX(x, y)       ((x) > (y) ? (x) : (y))

static GLuint PVR_TextureHeight(unsigned char *HDR);
static GLuint PVR_TextureWidth(unsigned char *HDR);
static GLuint PVR_TextureFormat(unsigned char *HDR);

static GLuint _glGetMipmapLevelCount(GLuint width, GLuint height) {
    return 1 + floor(log2(MAX(width, height)));
}

static GLuint _glGetMipmapDataSize(GLuint width, GLuint height) {
    GLuint size = 0;

    GLuint i = 0;

    for(; i < _glGetMipmapLevelCount(width, height); ++i) {
        size += (width * height * 2);

        if(width > 1) {
            width /= 2;
        }

        if(height > 1) {
            height /= 2;
        }
    }

    return size;
}

/* Load a PVR texture file into memory, and then bind the texture to Open GL.
   fname is the name of the PVR texture file to be opened and read.
   isMipMapped should be passed as 1 if the texture contains MipMap levels, 0 otherwise.
   glMipMap should be passed as 1 if Open GL should calculate the Mipmap levels, 0 otherwise */
GLuint glTextureLoadPVR(char *fname, unsigned char isMipMapped, unsigned char glMipMap) {
    FILE *tex = NULL;
    uint16_t *TEX0 = NULL;
    uint8_t HDR[PVR_HDR_SIZE];
    GLuint texID, texSize, texW, texH, texFormat;

    /* Open the PVR texture file, and get its file size */
    tex = fopen(fname, "rb");

    if(tex == NULL) {
        printf("FILE READ ERROR: %s\n", fname);

        while(1);
    }

    fseek(tex, 0, SEEK_END);
    texSize = ftell(tex) - PVR_HDR_SIZE;
    fseek(tex, 0, SEEK_SET);

    /* Read in the PVR texture file header */
    fread(HDR, 1, PVR_HDR_SIZE, tex);

    /* Extract some information from the PVR texture file header */
    texW = PVR_TextureWidth(HDR);
    texH = PVR_TextureHeight(HDR);
    texFormat = PVR_TextureFormat(HDR);

    /* Allocate Some Memory for the texture. If we are using Open GL to build the MipMap,
       we need to allocate enough space to hold the MipMap texture levels. */
    if(!isMipMapped && glMipMap)
        TEX0 = malloc(_glGetMipmapDataSize(texW, texH));
    else
        TEX0 = malloc(texSize);

    fread(TEX0, 1, texSize, tex); /* Read in the PVR texture data */
    fclose(tex);

    /* Generate and bind a texture as normal for Open GL */
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    if(texFormat != GL_UNSIGNED_SHORT_5_6_5)
        glCompressedTexImage2D(GL_TEXTURE_2D,
                           0,
 	                       texFormat,
 	                       texW,
 	                       texH,
 	                       0,
 	                       texSize,
 	                       TEX0);
    else {
        //fprintf(stderr, "%x\n", texFormat);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGB,
                     texW, texH,
                     0,
                     GL_RGB,
                     texFormat,
                     TEX0);
        if(!isMipMapped && glMipMap)
            glGenerateMipmapEXT(GL_TEXTURE_2D);
    }

    free(TEX0);

    return texID;
}

static GLuint PVR_TextureFormat(unsigned char *HDR) {
    GLuint color = (GLuint)HDR[PVR_HDR_SIZE - 8];
    GLuint format = (GLuint)HDR[PVR_HDR_SIZE - 7];

    GLboolean twiddled = format == 0x01;
    GLboolean compressed = (format == 0x10 || format == 0x03);

    if(compressed) {
        if(twiddled) {
            switch(color) {
                case 0x0: {
                    return GL_COMPRESSED_ARGB_1555_VQ_TWID_KOS;
                } break;
                case 0x01: {
                    return GL_COMPRESSED_RGB_565_VQ_TWID_KOS;
                } break;
                case 0x02: {
                    return GL_COMPRESSED_ARGB_4444_VQ_TWID_KOS;
                }
                break;
                default:
                    fprintf(stderr, "Invalid texture format");
                    return 0;
            }
        } else {
            switch(color) {
                case 0: {
                    return GL_COMPRESSED_ARGB_1555_VQ_KOS;
                } break;
                case 1: {
                    return GL_COMPRESSED_RGB_565_VQ_KOS;
                } break;
                case 2: {
                    return GL_COMPRESSED_ARGB_4444_VQ_KOS;
                }
                break;
                default:
                    fprintf(stderr, "Invalid texture format");
                    return 0;
            }
        }
    } else {
        if(color == 1) {
            return GL_UNSIGNED_SHORT_5_6_5;
        }
        return 0;
    }
}

static GLuint PVR_TextureWidth(unsigned char *HDR) {
    return (GLuint)HDR[PVR_HDR_SIZE - 4] | HDR[PVR_HDR_SIZE - 3] << 8;
}

static GLuint PVR_TextureHeight(unsigned char *HDR) {
    return (GLuint)HDR[PVR_HDR_SIZE - 2] | HDR[PVR_HDR_SIZE - 1] << 8;
}
