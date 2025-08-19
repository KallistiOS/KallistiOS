/* KallistiOS ##version##

   dc/pvr/pvr_dma.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2014 Lawrence Sebald
   Copyright (C) 2023 Ruslan Rostovtsev
   Copyright (C) 2024 Falco Girgis
*/

/** \file       dc/pvr/pvr_dma.h
    \brief      API for utilizing the DMA with the PVR for rendering
    \ingroup    pvr_dma

    \author Megan Potter
    \author Roger Cattermole
    \author Paul Boese
    \author Brian Paul
    \author Lawrence Sebald
    \author Benoit Miller
    \author Ruslan Rostovtsev
    \author Falco Girgis

    \todo
    - Top  level "Transfer" group
        a. DMA
        b. SQs
*/

#ifndef __DC_PVR_PVR_DMA_H
#define __DC_PVR_PVR_DMA_H

#include <stdint.h>
#include <stdbool.h>

#include <kos/cdefs.h>
__BEGIN_DECLS

/** \defgroup pvr_dma   DMA
    \brief              PowerVR DMA driver
    \ingroup            pvr
*/

/** \brief    Transfer modes with TA/PVR DMA and Store Queues
    \ingroup  pvr_dma
*/
typedef enum pvr_dma_type {
    PVR_DMA_VRAM64,    /**< Transfer to VRAM using TA bus */
    PVR_DMA_VRAM32,    /**< Transfer to VRAM using TA bus */
    PVR_DMA_TA,        /**< Transfer to the tile accelerator */
    PVR_DMA_YUV,       /**< Transfer to the YUV converter (TA) */
    PVR_DMA_VRAM32_SB, /**< Transfer to/from VRAM using PVR i/f */
    PVR_DMA_VRAM64_SB  /**< Transfer to/from VRAM using PVR i/f */
} pvr_dma_type_t;

/** \brief   PVR DMA interrupt callback type.
    \ingroup pvr_dma

    Functions that act as callbacks when DMA completes should be of this type.
    These functions will be called inside an interrupt context, so don't try to
    use anything that might stall.

    \param  data            User data passed in to the pvr_dma_transfer()
                            function.
*/
typedef void (*pvr_dma_callback_t)(void *data);

/** \brief   Perform a DMA transfer to the PVR RAM over 64-bit TA bus.
    \ingroup pvr_dma

    This function copies a block of data to the PVR or its memory via DMA. There
    are all kinds of constraints that must be fulfilled to actually do this, so
    make sure to read all the fine print with the parameter list.

    If a callback is specified, it will be called in an interrupt context, so
    keep that in mind in writing the callback.

    \param  src             Where to copy from. Must be 32-byte aligned.
    \param  dest            Where to copy to. Must be 32-byte aligned.
    \param  count           The number of bytes to copy. Must be a multiple of
                            32.
    \param  type            The type of DMA transfer to do (see list of modes).
    \param  block           True if you want the function to block until the
                            DMA completes.
    \param  callback        A function to call upon completion of the DMA.
    \param  cbdata          Data to pass to the callback function.
    \retval 0               On success.
    \retval -1              On failure. Sets errno as appropriate.

    \par    Error Conditions:
    \em     EINPROGRESS - DMA already in progress \n
    \em     EFAULT - dest is not 32-byte aligned \n
    \em     EIO - I/O error
*/
int pvr_dma_transfer(const void *src, uintptr_t dest, size_t count,
                     pvr_dma_type_t type, bool block,
                     pvr_dma_callback_t callback, void *cbdata);


/** \brief   Load a texture using TA DMA.
    \ingroup pvr_dma

    This is essentially a convenience wrapper for pvr_dma_transfer(), so all
    notes that apply to it also apply here.

    \param  src             Where to copy from. Must be 32-byte aligned.
    \param  dest            Where to copy to. Must be 32-byte aligned.
    \param  count           The number of bytes to copy. Must be a multiple of
                            32.
    \param  block           True if you want the function to block until the
                            DMA completes.
    \param  callback        A function to call upon completion of the DMA.
    \param  cbdata          Data to pass to the callback function.
    \retval 0               On success.
    \retval -1              On failure. Sets errno as appropriate.

    \par    Error Conditions:
    \em     EINPROGRESS - DMA already in progress \n
    \em     EFAULT - dest is not 32-byte aligned \n
    \em     EIO - I/O error
*/
int pvr_txr_load_dma(const void *src, pvr_ptr_t dest, size_t count, bool block,
                     pvr_dma_callback_t callback, void *cbdata);

/** \brief   Load vertex data to the TA using TA DMA.
    \ingroup pvr_dma

    This is essentially a convenience wrapper for pvr_dma_transfer(), so all
    notes that apply to it also apply here.

    \param  src             Where to copy from. Must be 32-byte aligned.
    \param  count           The number of bytes to copy. Must be a multiple of
                            32.
    \param  block           True if you want the function to block until the
                            DMA completes.
    \param  callback        A function to call upon completion of the DMA.
    \param  cbdata          Data to pass to the callback function.
    \retval 0               On success.
    \retval -1              On failure. Sets errno as appropriate.

    \par    Error Conditions:
    \em     EINPROGRESS - DMA already in progress \n
    \em     EFAULT - dest is not 32-byte aligned \n
    \em     EIO - I/O error
 */
int pvr_dma_load_ta(const void *src, size_t count, bool block,
                    pvr_dma_callback_t callback, void *cbdata);

/** \brief   Load yuv data to the YUV converter using TA DMA.
    \ingroup pvr_dma

    This is essentially a convenience wrapper for pvr_dma_transfer(), so all
    notes that apply to it also apply here.

    \param  src             Where to copy from. Must be 32-byte aligned.
    \param  count           The number of bytes to copy. Must be a multiple of
                            32.
    \param  block           True if you want the function to block until the
                            DMA completes.
    \param  callback        A function to call upon completion of the DMA.
    \param  cbdata          Data to pass to the callback function.
    \retval 0               On success.
    \retval -1              On failure. Sets errno as appropriate.

    \par    Error Conditions:
    \em     EINPROGRESS - DMA already in progress \n
    \em     EFAULT - dest is not 32-byte aligned \n
    \em     EIO - I/O error
*/
int pvr_dma_yuv_conv(const void *src, size_t count, bool block,
                     pvr_dma_callback_t callback, void *cbdata);

/** \brief   Is PVR DMA is inactive?
    \ingroup pvr_dma
    \return                 True if there is no PVR DMA active, thus a DMA
                            can begin or false if there is an active DMA.
*/
bool pvr_dma_ready(void);

/** \brief   Initialize TA/PVR DMA. 
    \ingroup pvr_dma
 */
void pvr_dma_init(void);

/** \brief   Shut down TA/PVR DMA. 
    \ingroup pvr_dma
 */
void pvr_dma_shutdown(void);

/** \brief   Copy a block of memory to VRAM
    \ingroup store_queues

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
void *pvr_sq_load(void *dest, const void *src,
                  size_t n, pvr_dma_type_t type);

/** \brief   Set a block of PVR memory to a 16-bit value.
    \ingroup store_queues

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
void *pvr_sq_set16(void *dest, uint32_t c, size_t n, pvr_dma_type_t type);

/** \brief   Set a block of PVR memory to a 32-bit value.
    \ingroup store_queues

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

    \sa pvr_sq_set16()
*/
void *pvr_sq_set32(void *dest, uint32_t c, size_t n, pvr_dma_type_t type);

__END_DECLS

#endif  /* __DC_PVR_PVR_DMA_H */
