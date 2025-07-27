/* KallistiOS ##version##

   dc/pvr/pvr_txr.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2014 Lawrence Sebald
   Copyright (C) 2023 Ruslan Rostovtsev
   Copyright (C) 2024 Falco Girgis
   Copyright (C) 2024 Andress Barajas
*/

/** \file       dc/pvr/pvr_txr.h
    \brief      Texture management with the PVR 3D API
    \ingroup    pvr_txr_mgmt

    \author Megan Potter
    \author Roger Cattermole
    \author Paul Boese
    \author Brian Paul
    \author Lawrence Sebald
    \author Benoit Miller
    \author Ruslan Rostovtsev
    \author Falco Girgis
*/

#ifndef __DC_PVR_PVR_TEXTURE_H
#define __DC_PVR_PVR_TEXTURE_H

#include <stdbool.h>
#include <stdint.h>

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/img.h>

/** \defgroup pvr_txr_mgmt      Texturing
    \brief                      API for managing PowerVR textures
    \ingroup                    pvr
    
    Helper functions for handling texture tasks of various kinds.
*/

/** \brief   Set the global stride width for non-power-of-two textures in PVR RAM.
    \ingroup pvr_txr_mgmt

    This function configures the register `PVR_TEXTURE_MODULO`, whose
    first five bits define the row width in VRAM for non-power-of-two
    textures. The setting applies to all textures rendered with the
    `PVR_TXRFMT_X32_STRIDE` flag in the same frame.

    The stride width configured here is **only supported for textures
    with widths that are multiples of 32 pixels** and up to a maximum
    of 992 pixels.

    \warning
    - Textures that are twiddled cannot use the `PVR_TXRFMT_X32_STRIDE`
      flag so the stride set here will not apply to them. This includes
      all paletted and mipmap textures.

    \param  texture_width   The width of the texture in pixels. Must be a
                            multiple of 32 and up to 992 pixels.

    \sa pvr_txr_get_stride()
*/
void pvr_txr_set_stride(size_t texture_width);

/** \brief   Get the current texture stride width in pixels as set in the PVR.
    \ingroup pvr_txr_mgmt

    This function reads the `PVR_TEXTURE_MODULO` register and calculates the
    texture stride width in pixels. The value returned is the width in pixels
    that has been configured for all textures using the `PVR_TXRFMT_X32_STRIDE`
    flag in the same frame.

    The stride width is computed by taking the current multiplier in
    `PVR_TEXTURE_MODULO` (which stores the width divided by 32), and
    multiplying it back by 32 to return the full width in pixels.

    \return                 The current texture stride width in pixels.
                            Or 0 if not set
    \sa pvr_txr_set_stride()
*/
size_t pvr_txr_get_stride(void);

/** \brief   Load raw texture data from an SH-4 buffer into PVR RAM.
    \ingroup pvr_txr_mgmt 

    This essentially just acts as a memcpy() from main RAM to PVR RAM, using
    the Store Queues and 64-bit TA bus.

    \param  src             The location in main RAM holding the texture.
    \param  dst             The location in PVR RAM to copy to.
    \param  count           The size of the texture in bytes (must be a multiple
                            of 32).
*/
void pvr_txr_load(const void *src, pvr_ptr_t dst, size_t count);

/** \defgroup pvr_txrload_constants     Flags
    \brief                              Texture loading constants
    \ingroup                            pvr_txr_mgmt

    These are constants for the flags parameter to pvr_txr_load_ex() or
    pvr_txr_load_kimg().

    @{
*/
#define PVR_TXRLOAD_4BPP            0x01    /**< \brief 4BPP format */
#define PVR_TXRLOAD_8BPP            0x02    /**< \brief 8BPP format */
#define PVR_TXRLOAD_16BPP           0x03    /**< \brief 16BPP format */
#define PVR_TXRLOAD_FMT_MASK        0x0f    /**< \brief Bits used for basic formats */

#define PVR_TXRLOAD_VQ_LOAD         0x10    /**< \brief Do VQ encoding (not supported yet, if ever) */
#define PVR_TXRLOAD_INVERT_Y        0x20    /**< \brief Invert the Y axis while loading */
#define PVR_TXRLOAD_FMT_VQ          0x40    /**< \brief Texture is already VQ encoded */
#define PVR_TXRLOAD_FMT_TWIDDLED    0x80    /**< \brief Texture is already twiddled */
#define PVR_TXRLOAD_FMT_NOTWIDDLE   0x80    /**< \brief Don't twiddle the texture while loading */
#define PVR_TXRLOAD_DMA             0x8000  /**< \brief Use DMA to load the texture */
#define PVR_TXRLOAD_NONBLOCK        0x4000  /**< \brief Use non-blocking loads (only for DMA) */
#define PVR_TXRLOAD_SQ              0x2000  /**< \brief Use Store Queues to load */
/** @} */

/** \brief   Load texture data from an SH-4 buffer into PVR RAM, twiddling it in
             the process.
    \ingroup pvr_txr_mgmt

    This function loads a texture to the PVR's RAM with the specified set of
    flags. It will currently always twiddle the data, whether you ask it to or
    not, and many of the parameters are just plain not supported at all...
    Pretty much the only supported flag, other than the format ones is the
    PVR_TXRLOAD_INVERT_Y one.

    This will be slower than using pvr_txr_load() in pretty much all cases, so
    unless you need to twiddle your texture, just use that instead.

    \param  src             The location to copy from.
    \param  dst             The location to copy to.
    \param  w               The width of the texture, in pixels.
    \param  h               The height of the texture, in pixels.
    \param  flags           Some set of flags, ORed together.

    \see    pvr_txrload_constants
*/
void pvr_txr_load_ex(const void *src, pvr_ptr_t dst,
                     uint32_t w, uint32_t h, uint32_t flags);

/** \brief   Load a KOS Platform Independent Image (subject to constraint
             checking).
    \ingroup pvr_txr_mgmt

    This function loads a KOS Platform Independent image to the PVR's RAM with
    the specified set of flags. This function, unlike pvr_txr_load_ex() supports
    everything in the flags available, other than what's explicitly marked as
    not supported.

    \param  img             The image to load.
    \param  dst             The location to copy to.
    \param  flags           Some set of flags, ORed together.

    \see    pvr_txrload_constants
    \note                   Unless you explicitly tell this function to not
                            twiddle the texture (by ORing
                            \ref PVR_TXRLOAD_FMT_NOTWIDDLE or it's equivalent
                            \ref PVR_TXRLOAD_FMT_TWIDDLED with flags), this
                            function will twiddle the texture while loading.
                            Keep that in mind when setting the texture format in
                            polygon headers later.
    \note                   You cannot specify both
                            \ref PVR_TXRLOAD_FMT_NOTWIDDLE (or equivalently
                            \ref PVR_TXRLOAD_FMT_TWIDDLED) and
                            \ref PVR_TXRLOAD_INVERT_Y in the flags.
    \note                   DMA and Store Queue based loading is not available
                            from this function if it twiddles the texture while
                            loading.
*/
void pvr_txr_load_kimg(const kos_img_t *img, pvr_ptr_t dst, uint32_t flags);

__END_DECLS
#endif  /* __DC_PVR_PVR_TEXTURE_H */
