/* KallistiOS ##version##

   kos/img.h
   Copyright (C) 2002 Megan Potter

*/

#ifndef __KOS_IMG_H
#define __KOS_IMG_H

/** \file    kos/img.h
    \brief   Platform-independent image type.
    \ingroup video_img

    This file provides a platform-independent image type that is designed to
    hold any sort of textures or other image data. This type contains a very
    basic description of the image data (width, height, pixel format), as well
    as the image data itself.

    All of the image-loading libraries in kos-ports should provide a function
    to load the image data into one of these types.

    \author Megan Potter
*/

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

/** \defgroup video_img  Images
    \brief               Platform-independent image representation
    \ingroup             video
*/

/** \brief   Platform-indpendent image type.
    \ingroup video_img

    You can use this type for textures or whatever you feel it's appropriate
    for. "width" and "height" are as you would expect. "format" has a lower-half
    which is platform-independent and used to basically describe the contained
    data; the upper-half is platform-dependent and can hold anything (so AND it
    off if you only want the bottom part).

    Note that in some of the more obscure formats (like the paletted formats)
    the data interpretation may be platform dependent. Thus we also provide a
    data length field.

    \headerfile kos/img.h
*/
typedef struct kos_img {
    void *data;             /**< \brief Image data in the specified format. */
    uint32_t w;             /**< \brief Width of the image. */
    uint32_t h;             /**< \brief Height of the image. */
    uint32_t fmt;           /**< \brief Format of the image data.
                             \see   kos_img_fmts
                             \see   kos_img_fmt_macros */
    uint32_t byte_count;    /**< \brief Length of the image data, in bytes. */
} kos_img_t;

/** \defgroup video_img_fmt Format
    \brief                  Video image formats
    \ingroup                video_img
*/

/** \defgroup   kos_img_fmt_macros  Accessors
    \brief                          Macros for accessing kos_image_t::fmt
    \ingroup                        video_img_fmt

    These macros provide easy access to the fmt field of a kos_img_t object.

    @{
*/
/** \brief  Read the platform-independent half of the format.

    This macro masks the format of a kos_img_t to give you just the lower half
    of the value, which contains the platform-independent half of the format.

    \param  x           An image format (fmt field of a kos_img_t).
    
    \return             The platform-independent half of the format.
*/
#define KOS_IMG_FMT_I(x) ((x) & 0xffff)

/** \brief  Read the platform-specific half of the format.

    This macro masks the format of a kos_img_t to give you just the upper half
    of the value, which contains the platform-specific half of the format.

    \param  x           An image format (fmt field of a kos_img_t).
    
    \return             The platform-specific half of the format.
*/
#define KOS_IMG_FMT_D(x) (((x) >> 16) & 0xffff)

/** \brief  Build a format value from a platform-independent half and a
            platform-specific half of the value.

    This macro combines the platform-independent and platform-specific portions
    of an image format into a value suitable for storing as the fmt field of a
    kos_img_t object.

    \param  i           The platform-independent half of the format.
    \param  d           The platform-specific half of the format. This should
                        not be pre-shifted.
    
    \return             A complete image format value, suitable for placing in
                        the fmt variable of a kos_img_t.
*/
#define KOS_IMG_FMT(i, d) ( ((i) & 0xffff) | (((d) & 0xffff) << 16) )

/** @} */

/** \defgroup   kos_img_fmts        Types
    \brief                          Video image format types
    \ingroup                        video_img_fmt

    This is the list of platform-independent image types that can be used as the
    lower-half of the fmt value for a kos_img_t.

    @{
*/
/** \brief  Undefined or uninitialized format. */
#define KOS_IMG_FMT_NONE        0x00

/** \brief  24-bpp interleaved R/G/B bytes. */
#define KOS_IMG_FMT_RGB888      0x01

/** \brief  32-bpp interleaved A/R/G/B bytes. */
#define KOS_IMG_FMT_ARGB8888    0x02

/** \brief  16-bpp interleaved R (5 bits), G (6 bits), B (5 bits). */
#define KOS_IMG_FMT_RGB565      0x03

/** \brief  16-bpp interleaved A/R/G/B (4 bits each). */
#define KOS_IMG_FMT_ARGB4444    0x04

/** \brief  16-bpp interleaved A (1 bit), R (5 bits), G (5 bits), B (5 bits).
    \note   This can also be used for RGB555 (with the top bit ignored). */
#define KOS_IMG_FMT_ARGB1555    0x05

/** \brief  Paletted, 4 bits per pixel (16 colors). */
#define KOS_IMG_FMT_PAL4BPP     0x06

/** \brief  Paletted, 8 bits per pixel (256 colors). */
#define KOS_IMG_FMT_PAL8BPP     0x07

/** \brief  8-bit Y (4 bits), U (2 bits), V (2 bits). */
#define KOS_IMG_FMT_YUV422      0x08

/** \brief  15-bpp interleaved B (5 bits), G (6 bits), R (5 bits). */
#define KOS_IMG_FMT_BGR565      0x09

/** \brief  32-bpp interleaved R/G/B/A bytes. */
#define KOS_IMG_FMT_RGBA8888    0x10

/** \brief  Basic format mask (not an actual format value). */
#define KOS_IMG_FMT_MASK        0xff

/** \brief  X axis of image data is inverted (stored right to left). */
#define KOS_IMG_INVERTED_X      0x0100

/** \brief  Y axis of image data is inverted (stored bottom to top). */
#define KOS_IMG_INVERTED_Y      0x0200

/** \brief  The image is not the owner of the image data buffer.

    This generally implies that the image data is stored in ROM and thus cannot
    be freed.
*/
#define KOS_IMG_NOT_OWNER       0x0400

/** @} */

/** \brief   Free a kos_img_t object.
    \ingroup video_img

    This function frees the data in a kos_img_t object, returning any memory to
    the heap as appropriate. Optionally, this can also free the object itself,
    if required.

    \param  img             The image object to free.
    \param  struct_also     Set to non-zero to free the image object itself,
                            as well as any data contained therein.
*/
void kos_img_free(kos_img_t *img, int struct_also);

__END_DECLS

#endif  /* __KOS_IMG_H */

