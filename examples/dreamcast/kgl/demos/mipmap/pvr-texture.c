/*
   KallistiOS 2.0.0

   pvr-texture.c
   (c)2014 Josh PH3NOM Pearson

   Load A PVR Texture to the PVR using Open GL
*/

#include <kos.h>
#include <KGL/gl.h>
#include <KGL/glu.h>

#define PVR_HDR_SIZE 0x20

static GLuint PVR_TextureHeight(unsigned char *HDR);
static GLuint PVR_TextureWidth(unsigned char *HDR);
static GLuint PVR_TextureFormat(unsigned char *HDR);
static GLuint PVR_TextureColor(unsigned char *HDR);

/* Load a PVR texture file into memory, and then bind the texture to Open GL.
   fname is the name of the PVR texture file to be opened and read.
   isMipMapped should be passed as 1 if the texture contains MipMap levels, 0 otherwise.
   glMipMap should be passed as 1 if Open GL should calculate the Mipmap levels, 0 otherwise */
GLuint glTextureLoadPVR(char *fname, unsigned char isMipMapped, unsigned char glMipMap) {
    FILE *tex = NULL;
    uint16 *TEX0 = NULL;
    uint8 HDR[PVR_HDR_SIZE];
    GLuint texID, texSize, texW, texH, texFormat, texColor;

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
    texColor = PVR_TextureColor(HDR);

    /* Allocate Some Memory for the texture. If we are using Open GL to build the MipMap,
       we need to allocate enough space to hold the MipMap texture levels. */
    if(!isMipMapped && glMipMap)
        TEX0 = malloc(glKosMipMapTexSize(texW, texH));
    else
        TEX0 = malloc(texSize);

    fread(TEX0, 1, texSize, tex); /* Read in the PVR texture data */

    /* If requested, tell Open GL to build the MipMap levels for the texture. */
    /* For now, the input texture to gluBuild2DMipmaps must have a width and height divisible by two. */
    /* Also, color format is only set by the 2nd to last parameter, here as texColor.
       The color format may be one of the three: PVR_TXRFMT_RGB565, PVR_TXRFMT_ARGB1555, or PVR_TXRFMT_ARGB4444 */
    if(!isMipMapped && glMipMap)
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, texW, texH, GL_RGB, texColor, TEX0);

    /* Generate and bind a texture as normal for Open GL */
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    if(texFormat & PVR_TXRFMT_VQ_ENABLE)
        glCompressedTexImage2D(GL_TEXTURE_2D,
                               (isMipMapped || glMipMap) ? 1 : 0,
 	                       texFormat | texColor,
 	                       texW,
 	                       texH,
 	                       0,
 	                       texSize,
 	                       TEX0);
    else
        glTexImage2D(GL_TEXTURE_2D,
                     (isMipMapped || glMipMap) ? 1 : 0,
                     GL_RGB,
                     texW, texH,
                     0,
                     GL_RGB,
                     texFormat | texColor,
                     TEX0);    
    
    free(TEX0);

    return texID;
}

static GLuint PVR_TextureColor(unsigned char *HDR) {
    switch((GLuint)HDR[PVR_HDR_SIZE - 8]) {
        case 0x00:
            return PVR_TXRFMT_ARGB1555; //(bilevel translucent alpha 0,255)

        case 0x01:
            return PVR_TXRFMT_RGB565;   //(non translucent RGB565 )

        case 0x02:
            return PVR_TXRFMT_ARGB4444; //(translucent alpha 0-255)

        case 0x03:
            return PVR_TXRFMT_YUV422;   //(non translucent UYVY )

        case 0x04:
            return PVR_TXRFMT_BUMP;     //(special bump-mapping format)

        case 0x05:
            return PVR_TXRFMT_PAL4BPP;  //(4-bit palleted texture)

        case 0x06:
            return PVR_TXRFMT_PAL8BPP;  //(8-bit palleted texture)

        default:
            return PVR_TXRFMT_RGB565;
    }
}

static GLuint PVR_TextureFormat(unsigned char *HDR) {
    switch((GLuint)HDR[PVR_HDR_SIZE - 7]) {
        case 0x01:
            return PVR_TXRFMT_TWIDDLED;                           //SQUARE TWIDDLED

        case 0x03:
            return PVR_TXRFMT_VQ_ENABLE;                          //VQ TWIDDLED

        case 0x09:
            return PVR_TXRFMT_NONTWIDDLED;                        //RECTANGLE

        case 0x0B:
            return PVR_TXRFMT_X32_STRIDE | PVR_TXRFMT_NONTWIDDLED;    //RECTANGULAR STRIDE

        case 0x0D:
            return PVR_TXRFMT_TWIDDLED;                           //RECTANGULAR TWIDDLED

        case 0x10:
            return PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_NONTWIDDLED; //SMALL VQ

        default:
            return PVR_TXRFMT_NONE;
    }
}

static GLuint PVR_TextureWidth(unsigned char *HDR) {
    return (GLuint)HDR[PVR_HDR_SIZE - 4] | HDR[PVR_HDR_SIZE - 3] << 8;
}

static GLuint PVR_TextureHeight(unsigned char *HDR) {
    return (GLuint)HDR[PVR_HDR_SIZE - 2] | HDR[PVR_HDR_SIZE - 1] << 8;
}
