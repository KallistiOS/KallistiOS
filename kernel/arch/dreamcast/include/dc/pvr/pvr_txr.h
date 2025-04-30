/* KallistiOS ##version##

   dc/pvr/pvr_txr.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2014 Lawrence Sebald
   Copyright (C) 2023 Ruslan Rostovtsev
   Copyright (C) 2024 Falco Girgis
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

#include <stdint.h>

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/img.h>

/** \defgroup pvr_txr_mgmt      Texturing
    \brief                      API for managing PowerVR textures
    \ingroup                    pvr
    
    Helper functions for handling texture tasks of various kinds.
*/

/** \brief   Load raw texture data from an SH-4 buffer into PVR RAM.
    \ingroup pvr_txr_mgmt 

    This essentially just acts as a memcpy() from main RAM to PVR RAM, using
    the Store Queues and 64-bit TA bus.

    \param  src             The location in main RAM holding the texture.
    \param  dst             The location in PVR RAM to copy to.
    \param  count           The size of the texture in bytes (must be a multiple
                            of 32).
*/
void pvr_txr_load(const void *src, pvr_ptr_t dst, uint32_t count);

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
