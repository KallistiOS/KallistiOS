/* KallistiOS ##version##

   kos/pcx.h
   Copyright (C) 2000-2001 Megan Potter

*/

#ifndef __KOS_PCX_H
#define __KOS_PCX_H

/** \file   kos/pcx.h
    \brief  Small PCX Loader.

    This module provides a few functions used for loading PCX files. These
    functions were mainly for use on the GBA port of KallistiOS (which has been
    removed from the tree), although they can be used pretty much anywhere.
    That said, libpcx is generally more useful than these functions for use on
    the Dreamcast.

    \author Megan Potter
*/

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/* These first two versions are mainly for use with the GBA, and they are
   defined in the "pcx_small" file. They can be used on any architecture of
   course. */

/** \brief  Load a PCX file into a buffer as flat 15-bit BGR data.

    This function loads the specified PCX file into the buffer provided,
    decoding the image to 15-bit BGR pixels. The width and height of the buffer
    should match that of the image itself.

    \param  fn          The file to load.
    \param  w_out       Buffer to return the width of the image in.
    \param  h_out       Buffer to return the height of the image in.
    \param  pic_out     Buffer to store image data in.
    \return             0 on success, < 0 on failure.
*/
int pcx_load_flat(const char *fn, int *w_out, int *h_out, void *pic_out);

/** \brief  Load a PCX file into a buffer as paletted data.

    This function is similar to the pcx_load_flat() function, but instead of
    decoding the image into a flat buffer, it decodes the image into a paletted
    format. The image itself will be 8-bit paletted pixels, and the palette is
    in a 15-bit BGR format. The width and height of the buffer should match that
    of the image itself.

    \param  fn          The file to load.
    \param  w_out       Buffer to return the width of the image in.
    \param  h_out       Buffer to return the height of the image in.
    \param  pic_out     Buffer to store image data in. This should be width *
                        height bytes in size.
    \param  pal_out     Buffer to store the palette data in. This should be
                        allocated to hold 256 uint16_t values.
    \return             0 on success, < 0 on failure.
*/
int pcx_load_palette(const char *fn, int *w_out, int *h_out, void *pic_out,
                     void *pal_out);

__END_DECLS

#endif  /* __KOS_PCX_H */

