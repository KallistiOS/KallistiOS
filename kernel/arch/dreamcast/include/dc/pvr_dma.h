/* KallistiOS ##version##

    dc/pvr_dma.h
    Copyright (C) 2002 Megan Potter
    Copyright (C) 2014 Lawrence Sebald
    Copyright (C) 2023 Ruslan Rostovtsev
    Copyright (C) 2024 Andress Barajas
*/

/** \file    dc/pvr_dma.h
    \brief   PVR/TA DMA interface.
    \ingroup pvr

    This file provides support for PVR DMA and TA DMA transfers in the 
    Dreamcast. These DMA types can be used independently of each other, 
    and both can operate concurrently. Depending on the transfer method 
    specified, the appropriate DMA mechanism is selected.

    TA DMA (Tile Accelerator DMA): This is a one-way transfer method used 
    primarily for submitting data directly to the Tile Accelerator (TA). It is 
    commonly used for submitting vertex data, texture data, and YUV conversion 
    data.

    PVR DMA (PowerVR DMA): This is a more flexible, two-way transfer mechanism 
    that supports both uploading and downloading data. PVR DMA is used for 
    transferring textures and palette data to VRAM, downloading data from VRAM 
    to the SH4, and for handling register data (e. palettes, fog tables).

    \author Megan Potter
    \author Roger Cattermole
    \author Paul Boese
    \author Brian Paul
    \author Lawrence Sebald
    \author Benoit Miller
    \author Ruslan Rostovtsev
*/

#ifndef __DC_PVR_DMA_H
#define __DC_PVR_DMA_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <dc/sq.h>

/* Forward declare pvr_ptr_t to avoid including pvr.h */
typedef void *pvr_ptr_t;

/** \defgroup pvr_dma   DMA
    \brief              PowerVR DMA driver
    \ingroup            pvr

    @{
*/

/** \defgroup pvr_dma_modes   Transfer Modes
    \brief                    Transfer modes with TA/PVR DMA and Store Queues

    @{
*/
typedef enum {
    PVR_DMA_VRAM64 = 0,    /**< \brief Transfer to VRAM using TA bus */
    PVR_DMA_VRAM32 = 1,    /**< \brief Transfer to VRAM using TA bus */
    PVR_DMA_TA = 2,        /**< \brief Transfer to the tile accelerator */
    PVR_DMA_YUV = 3,       /**< \brief Transfer to the YUV converter (TA) */
    PVR_DMA_VRAM32_SB = 4, /**< \brief Transfer to/from VRAM using PVR i/f */
    PVR_DMA_VRAM64_SB = 5, /**< \brief Transfer to/from VRAM using PVR i/f */
    PVR_DMA_REGISTERS = 6  /**< \brief Transfer to/from PVR registers */
} pvr_dma_mode_t;
/** @} */

/** \brief   PVR DMA interrupt callback type.

    Functions that act as callbacks when DMA completes should be of this type.
    These functions will be called inside an interrupt context, so don't try to
    use anything that might stall.

    \param  data            User data passed in to the pvr_dma_transfer()
                            function.
*/
typedef void (*pvr_dma_callback_t)(void *data);

/** \brief   Perform a DMA transfer to/from the TA/PVR RAM.

    This function copies a block of data to/from the TA/PVR or its memory via 
    DMA. There are all kinds of constraints that must be fulfilled to actually 
    do this, so make sure to read all the fine print with the parameter list.

    If a callback is specified, it will be called in an interrupt context, so
    keep that in mind in writing the callback.

    \param  src             Where to copy from. Must be 32-byte aligned.
    \param  dest            Where to copy to. Must be 32-byte aligned.
    \param  count           The number of bytes to copy. Must be a multiple of
                            32.
    \param  type            The type of DMA transfer to do (see list of modes).
    \param  block           Non-zero if you want the function to block until the
                            DMA completes.
    \param  callback        A function to call upon completion of the DMA.
    \param  cbdata          Data to pass to the callback function.
    \retval 0               On success.
    \retval -1              On failure. Sets errno as appropriate.

    \par    Error Conditions:
    \em     EINPROGRESS - DMA already in progress \n
    \em     EFAULT - src or dest is not 32-byte aligned \n
    \em     EIO - I/O error

    \see    pvr_dma_modes
*/
int pvr_dma_transfer(void *src, void *dest, size_t count, pvr_dma_mode_t type,
                     int block, pvr_dma_callback_t callback, void *cbdata);

/** \brief   Load a texture using TA DMA.

    This is essentially a convenience wrapper for pvr_dma_transfer(), so all
    notes that apply to it also apply here.

    \param  src             Where to copy from. Must be 32-byte aligned.
    \param  dest            Where to copy to. Must be 32-byte aligned.
    \param  count           The number of bytes to copy. Must be a multiple of
                            32.
    \param  block           Non-zero if you want the function to block until the
                            DMA completes.
    \param  callback        A function to call upon completion of the DMA.
    \param  cbdata          Data to pass to the callback function.
    \retval 0               On success.
    \retval -1              On failure. Sets errno as appropriate.

    \par    Error Conditions:
    \em     EINPROGRESS - DMA already in progress \n
    \em     EFAULT - src or dest is not 32-byte aligned \n
    \em     EIO - I/O error
*/
int pvr_txr_load_dma(void *src, pvr_ptr_t dest, size_t count, int block,
                     pvr_dma_callback_t callback, void *cbdata);

/** \brief   Load a texture using PVR DMA.

    This is essentially a convenience wrapper for pvr_dma_transfer(), so all
    notes that apply to it also apply here.

    \param  src             Where to copy from. Must be 32-byte aligned.
    \param  dest            Where to copy to. Must be 32-byte aligned.
    \param  count           The number of bytes to copy. Must be a multiple of
                            32.
    \param  block           Non-zero if you want the function to block until the
                            DMA completes.
    \param  callback        A function to call upon completion of the DMA.
    \param  cbdata          Data to pass to the callback function.
    \retval 0               On success.
    \retval -1              On failure. Sets errno as appropriate.

    \par    Error Conditions:
    \em     EINPROGRESS - DMA already in progress \n
    \em     EFAULT - src or dest is not 32-byte aligned \n
    \em     EIO - I/O error
*/
int pvr_dma_load_txr(void *src, pvr_ptr_t dest, size_t count, int block, 
                    pvr_dma_callback_t callback, void *cbdata);

/** \brief   Download a texture using PVR DMA.

    This is essentially a convenience wrapper for pvr_dma_transfer(), so all
    notes that apply to it also apply here.

    \param  src             Where to copy to. Must be 32-byte aligned.
    \param  dest            Where to copy from. Must be 32-byte aligned.
    \param  count           The number of bytes to copy. Must be a multiple of
                            32.
    \param  block           Non-zero if you want the function to block until the
                            DMA completes.
    \param  callback        A function to call upon completion of the DMA.
    \param  cbdata          Data to pass to the callback function.
    \retval 0               On success.
    \retval -1              On failure. Sets errno as appropriate.

    \par    Error Conditions:
    \em     EINPROGRESS - DMA already in progress \n
    \em     EFAULT - src or dest is not 32-byte aligned \n
    \em     EIO - I/O error
*/
int pvr_dma_download_txr(pvr_ptr_t src, void *dest, size_t count, int block, 
                        pvr_dma_callback_t callback, void *cbdata);

/** \brief   Load vertex data to the TA using TA DMA.

    This is essentially a convenience wrapper for pvr_dma_transfer(), so all
    notes that apply to it also apply here.

    \param  src             Where to copy from. Must be 32-byte aligned.
    \param  count           The number of bytes to copy. Must be a multiple of
                            32.
    \param  block           Non-zero if you want the function to block until the
                            DMA completes.
    \param  callback        A function to call upon completion of the DMA.
    \param  cbdata          Data to pass to the callback function.
    \retval 0               On success.
    \retval -1              On failure. Sets errno as appropriate.

    \par    Error Conditions:
    \em     EINPROGRESS - DMA already in progress \n
    \em     EFAULT - src is not 32-byte aligned \n
    \em     EIO - I/O error
 */
int pvr_dma_load_ta(void *src, size_t count, int block,
                    pvr_dma_callback_t callback, void *cbdata);

/** \brief   Load yuv data to the YUV converter using TA DMA.

    This is essentially a convenience wrapper for pvr_dma_transfer(), so all
    notes that apply to it also apply here.

    \param  src             Where to copy from. Must be 32-byte aligned.
    \param  count           The number of bytes to copy. Must be a multiple of
                            32.
    \param  block           Non-zero if you want the function to block until the
                            DMA completes.
    \param  callback        A function to call upon completion of the DMA.
    \param  cbdata          Data to pass to the callback function.
    \retval 0               On success.
    \retval -1              On failure. Sets errno as appropriate.

    \par    Error Conditions:
    \em     EINPROGRESS - DMA already in progress \n
    \em     EFAULT - src is not 32-byte aligned \n
    \em     EIO - I/O error
*/
int pvr_dma_yuv_conv(void *src, size_t count, int block,
                     pvr_dma_callback_t callback, void *cbdata);

/** \brief   Checks if the TA DMA is inactive.
    
    \deprecated
    This function is formally deprecated, and should not be used in newly
    written code. Instead, please use pvr_dma_ta_ready().

    \return                 Non-zero if there is no TA DMA active, thus a DMA
                            can begin or 0 if there is an active DMA.
*/
int pvr_dma_ready(void)  __depr("Use pvr_dma_ta_ready instead.");

/** \brief   Checks if the TA DMA is inactive.
    \return                 Non-zero if there is no TA DMA active, thus a DMA
                            can begin or 0 if there is an active DMA.
*/
int pvr_dma_ta_ready(void);

/** \brief   Checks if the PVR DMA is inactive.
    \return                 Non-zero if there is no PVR DMA active, thus a DMA
                            can begin or 0 if there is an active DMA.
*/
int pvr_dma_rb_ready(void);

/** \brief   Initialize TA/PVR DMA. */
void pvr_dma_init(void);

/** \brief   Shut down TA/PVR DMA. */
void pvr_dma_shutdown(void);

/** \brief   Copy a block of memory to VRAM

    This function is similar to sq_cpy(), but it has been
    optimized for writing to a destination residing within VRAM.

    \warning
    This function cannot be used at the same time as a PVR DMA transfer.

    The dest pointer must be at least 32-byte aligned and reside 
    in video memory, the src pointer must be at least 8-byte aligned, 
    and n must be a multiple of 32.

    \param  dest            The address to copy to (32-byte aligned).
    \param  src             The address to copy from (32-bit (8-byte) aligned).
    \param  n               The number of bytes to copy (multiple of 32).
    \param  type            The type of SQ/DMA transfer to do (see list of modes).
    \return                 The original value of dest.

    \sa pvr_sq_set32()
*/
void *pvr_sq_load(void *dest, const void *src, size_t n, pvr_dma_mode_t type);

/** \brief   Set a block of PVR memory to a 16-bit value.

    This function is similar to sq_set16(), but it has been
    optimized for writing to a destination residing within VRAM.

    \warning
    This function cannot be used at the same time as a PVR DMA transfer.
    
    The dest pointer must be at least 32-byte aligned and reside in video 
    memory, n must be a multiple of 32 and only the low 16-bits are used 
    from c.

    \param  dest            The address to begin setting at (32-byte aligned).
    \param  c               The value to set (in the low 16-bits).
    \param  n               The number of bytes to set (multiple of 32).
    \param  type            The type of SQ/DMA transfer to do (see list of modes).
    \return                 The original value of dest.

    \sa pvr_sq_set32()
*/
void *pvr_sq_set16(void *dest, uint32_t c, size_t n, pvr_dma_mode_t type);

/** \brief   Set a block of PVR memory to a 32-bit value.

    This function is similar to sq_set32(), but it has been
    optimized for writing to a destination residing within VRAM.

    \warning
    This function cannot be used at the same time as a PVR DMA transfer.

    The dest pointer must be at least 32-byte aligned and reside in video 
    memory, n must be a multiple of 32.

    \param  dest            The address to begin setting at (32-byte aligned).
    \param  c               The value to set.
    \param  n               The number of bytes to set (multiple of 32).
    \param  type            The type of SQ/DMA transfer to do (see list of modes).
    \return                 The original value of dest.

    \sa pvr_sq_set16
*/
void *pvr_sq_set32(void *dest, uint32_t c, size_t n, pvr_dma_mode_t type);

/** @} */

__END_DECLS

#endif /* __DC_PVR_DMA_H */
