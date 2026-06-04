/* KallistiOS ##version##

   kmg/kmg.h
   Copyrigh (C)2003 Megan Potter
*/

#ifndef __KMG_KMG_H
#define __KMG_KMG_H

#ifdef _arch_dreamcast
#   include <sys/cdefs.h>
    __BEGIN_DECLS
#endif

#include <stdint.h>
#include <kos/img.h>

/* Header for KMG files. This isn't particularly meant to be future-proof
   since generally you'll build your textures into this format and then
   pair that data with an executable build. For safety though, we go ahead
   and put a version number anyway. All fields are _little endian_. */
typedef struct kmg_header {
    uint32_t        magic;          /* Magic code */
    uint32_t        version;        /* Version code */
    uint32_t        platform;       /* Platform specifier (major format) */
    uint32_t        format;         /* Image (minor) format spec */
    uint32_t        width;          /* Image width */
    uint32_t        height;         /* Image height */
    uint32_t        byte_count;     /* Image's data size in bytes */
    uint8_t         padding[36];    /* Pad to a 64-byte header (all zeros) */
} kmg_header_t;

/* Magic code -- every KMG will start with one of these. */
#define KMG_MAGIC   0x00474d4b  /* 'KMG\0' */

/* Version specifier -- for this version of the lib. A new version
   means the files are incompatible. */
#define KMG_VERSION 1

/* Platform specifiers */
#define KMG_PLAT_DC     1
#define KMG_PLAT_GBA    2
#define KMG_PLAT_PS2    3

/* Format specifiers for DC */
#define KMG_DCFMT_4BPP_PAL  0x01    /* Paletted formats */
#define KMG_DCFMT_8BPP_PAL  0x02
#define KMG_DCFMT_RGB565    0x03    /* True-color formats */
#define KMG_DCFMT_ARGB4444  0x04
#define KMG_DCFMT_ARGB1555  0x05
#define KMG_DCFMT_YUV422    0x06
#define KMG_DCFMT_BUMP      0x07
#define KMG_DCFMT_MASK      0xff

#define KMG_DCFMT_VQ        0x0100  /* VQ-encoded (incl codebook) */
#define KMG_DCFMT_TWIDDLED  0x0200  /* Pre-twiddled */
#define KMG_DCFMT_MIPMAP    0x0400  /* Includes mipmaps */

#ifdef _arch_dreamcast

    /* Call to load a KMG file from the VFS. */
    int kmg_to_img(const char *fn, kos_img_t *rv);


    __END_DECLS
#endif

#endif  /* __KMG_KMG_H */

