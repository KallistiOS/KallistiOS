/* KallistiOS ##version##

   dmac.h
   Copyright (C) 2024 Paul Cercueil

*/

/** \file    arch/dmac.h
    \brief   SH4 DMA Controller API
    \ingroup system_dmac

    This header provies an API to use the DMA controller of the SH4.

    \author Paul Cercueil
*/

#ifndef __ARCH_DMAC_H
#define __ARCH_DMAC_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <stdbool.h>

/** \defgroup dmac  DMA Controller API
    \brief          API to use the SH4's DMA Controller
    \ingroup        system

    This API can be used to program DMA transfers from memory to memory,
    from hardware to memory or from memory to hardware.

    @{
*/

/** \brief   DMA callback type.
    \ingroup dmac

    Function type for DMA callbacks.
    Those registered callbacks will be called when a DMA transfer completes.
    They will be called in an interrupt context, so don't try anything funny.
*/
typedef void (*dma_callback_t)(void *data);

/** \brief   DMA channel enum.
    \ingroup dmac

    Represents one of the 4 DMA channels available on the SH4.
*/
typedef enum dma_channel {
    DMA_CHANNEL_0, /**< Channel #0: On Dreamcast, reserved for hardware. */
    DMA_CHANNEL_1, /**< Channel #1: External, hardware or mem-to-mem requests. */
    DMA_CHANNEL_2, /**< Channel #2: on Dreamcast, reserved for PVR use. */
    DMA_CHANNEL_3, /**< Channel #3: mem-to-mem requests only. */
} dma_channel_t;

/** \brief   DMA request.
    \ingroup dmac

    List of the possible DMA requests.

    "Auto" requests are started as soon as the DMA transfer is programmed to
    the DMA controller. On the other hand, SCI/SCIF/TMU2 requests are only
    started when the given hardware event occurs.

    "External" requests are controlled by external pins of the SH4 CPU. These
    can be wired to other parts of the mother board.
*/
typedef enum dma_request {
    DMA_REQUEST_EXTERNAL_MEM_TO_MEM = 0,
    DMA_REQUEST_EXTERNAL_MEM_TO_DEV = 2,
    DMA_REQUEST_EXTERNAL_DEV_TO_MEM = 3,
    DMA_REQUEST_AUTO_MEM_TO_MEM = 4,
    DMA_REQUEST_AUTO_MEM_TO_DEV = 5,
    DMA_REQUEST_AUTO_DEV_TO_MEM = 6,

    DMA_REQUEST_SCI_TRANSMIT = 8,
    DMA_REQUEST_SCI_RECEIVE = 9,
    DMA_REQUEST_SCIF_TRANSMIT = 10,
    DMA_REQUEST_SCIF_RECEIVE = 11,
    DMA_REQUEST_TMU2_MEM_TO_MEM = 12,
    DMA_REQUEST_TMU2_MEM_TO_DEV = 13,
    DMA_REQUEST_TMU2_DEV_TO_MEM = 14,
} dma_request_t;

/** \brief   DMA unit size.
    \ingroup dmac

    The unit size controls the granularity at which the DMA will transfer data.
    For instance, copying data to a 16-bit bus will require a 16-bit unit size.

    For memory-to-memory transfers, it is recommended to use 32-byte transfers
    for the maximum speed.
*/
typedef enum dma_unitsize {
    DMA_UNITSIZE_64BIT,
    DMA_UNITSIZE_8BIT,
    DMA_UNITSIZE_16BIT,
    DMA_UNITSIZE_32BIT,
    DMA_UNITSIZE_32BYTE,
} dma_unitsize_t;

/** \brief   DMA address mode.
    \ingroup dmac

    The "address mode" specifies how the source or destination address of a DMA
    transfer is modified as the transfer goes on. It is only valid when the DMA
    transfer is configured as pointing to memory for that source or destination.
*/
typedef enum dma_addrmode {
    DMA_ADDRMODE_FIXED,     /**< The source/destination address is not modified. */
    DMA_ADDRMODE_INCREMENT, /**< The source/destination address is incremented. */
    DMA_ADDRMODE_DECREMENT, /**< The source/destination address is decremented. */
} dma_addrmode_t;

/** \brief   DMA transmit mode.
    \ingroup dmac

    In "Cycle steal" mode, the DMA controller will release the bus at the end of
    each transfer unit (configured by the unit size). This allows the CPU to
    access the bus if it needs to.

    In "Burst" mode, the DMA controller will hold the bus until the DMA transfer
    completes.
*/
typedef enum dma_transmitmode {
    DMA_TRANSMITMODE_CYCLE_STEAL,
    DMA_TRANSMITMODE_BURST,
} dma_transmitmode_t;

/** \brief   DMA transfer configuration.
    \ingroup dmac

    This structure represents the configuration used for a given DMA channel.
*/
typedef struct dma_config {
    dma_channel_t channel;              /**< DMA channel used for the transfer. */
    dma_request_t request;              /**< DMA request type. */
    dma_unitsize_t unit_size;           /**< Unit size used for the DMA transfer. */
    dma_addrmode_t src_mode, dst_mode;  /**< Source/destination address mode. */
    dma_transmitmode_t transmit_mode;   /**< DMA Transfer transmit mode. */
    dma_callback_t callback;            /**< Optional callback function for
                                             end-of-transfer notification. */
} dma_config_t;

/** \brief   DMA address.
    \ingroup dmac

    This type represents an address that can be used for DMA transfers.
*/
typedef uint32_t dma_addr_t;

/** \brief   Convert a hardware address to a DMA address.
    \ingroup dmac

    This function will convert a hardware address (pointing to a device's FIFO,
    or to one of the various mapped memories) to an address suitable for use
    with the DMA controller.

    \param  hw_addr         The hardware address.
    \return                 The converted DMA address.
*/
dma_addr_t hw_to_dma_addr(uintptr_t hw_addr);

/** \brief   Prepare a source memory buffer for a DMA transfer.
    \ingroup dmac

    This function will flush the data cache for the memory area covered by the
    buffer to ensure memory coherency, and return an address suitable for use
    with the DMA controller.

    \param  ptr             A pointer to the source buffer.
    \param  len             The size in bytes of the source buffer.
    \return                 The converted DMA address.

    \sa dma_map_dst()
*/
dma_addr_t dma_map_src(const void *ptr, size_t len);

/** \brief   Prepare a destination memory buffer for a DMA transfer.
    \ingroup dmac

    This function will invalidate the data cache for the memory area covered by
    the buffer to ensure memory coherency, and return an address suitable for
    use with the DMA controller.

    \param  ptr             A pointer to the destination buffer.
    \param  len             The size in bytes of the destination buffer.
    \return                 The converted DMA address.

    \sa dma_map_src()
*/
dma_addr_t dma_map_dst(void *ptr, size_t len);

/** \brief   Program a DMA transfer.
    \ingroup dmac

    This function will program a DMA transfer using the specified configuration,
    source/destination addresses and transfer length.

    It will return as soon as the DMA transfer is programmed, which means that
    it will not work for the DMA transfer to complete before returning.

    \param  cfg             A pointer to the configuration structure.
    \param  dst             The destination address, if targetting memory;
                            can be 0 otherwise.
    \param  src             The source address, if targetting memory;
                            can be 0 otherwise.
    \param  len             The size in bytes of the DMA transfer.
    \param  cb_data         The parameter that will be passed to the
                            end-of-transfer callback, if a callback has been
                            specified in the configuration structure.
    \retval 0               On success.
    \retval -1              On error.

    \sa dma_wait_complete()
*/
int dma_transfer(const dma_config_t *cfg, dma_addr_t dst, dma_addr_t src,
                 size_t len, void *cb_data);

/** \brief   Wait for a DMA transfer to complete
    \ingroup dmac

    This function will block until any previously programmed DMA transfer for
    the given DMA channel has completed.

    \param  channel         The DMA channel to wait for.
*/
void dma_wait_complete(dma_channel_t channel);

/** \brief   Check if a DMA transfer is running
    \ingroup dmac

    This function will return true if a DMA transfer is running for the given
    DMA channel.

    \param  channel         The DMA channel to check.
    \return                 True if a DMA transfer is running, false otherwise.
*/
bool dma_is_running(dma_channel_t channel);

/** \brief   Get the remaining size of a DMA transfer
    \ingroup dmac

    This function will return the number of bytes remaining to transfer, if a
    transfer was previously programmed.

    \param  channel         The DMA channel to wait for.
    \return                 The number of bytes remaining to transfer, or zero
                            if the previous transfer completed.
*/
size_t dma_transfer_get_remaining(dma_channel_t channel);

/** \brief   Abort a DMA transfer
    \ingroup dmac

    This function will abort a DMA transfer for the given DMA channel.

    \param  channel         The DMA channel to abort.
*/
void dma_transfer_abort(dma_channel_t channel);

/** @} */

__END_DECLS

#endif /* __ARCH_DMAC_H */
