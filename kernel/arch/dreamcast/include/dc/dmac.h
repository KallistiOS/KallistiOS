/* KallistiOS ##version##

   dmac.h
   Copyright (C) 2023 Andy Barajas

*/

/** \file    dc/dmac.h
    \brief   Macros to access the DMA controller registers. 
    \ingroup system_dmac

    This header provides a set of macros to facilitate checking
    the values of various DMA channels on the system.

    DMA channel 0 and its registers (DMAC_SAR0, DMAC_DAR0, DMAC_DMATCR0, 
    DMAC_CHCR0) are used by the hardware and not accessible to us but are 
    documented here anyway.

    DMA channel 2 is strictly used to transfer data to the PVR/TA.

    DMA channel 1 & 3 are free to use.
    
    \author Andy Barajas
*/

#ifndef __DC_DMAC_H
#define __DC_DMAC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \defgroup system_dmac   DMA
    \brief                  Driver for the SH4's Direct Memory Access 
                            Controller
    \ingroup                system

    @{
*/

#define DMAC_BASE     0xffa00000

/** \name DMA Source Address Registers (SAR0-SAR3)
  
    These registers designate the source address for DMA transfers. 
    Currently we only support 32-byte boundary addresses.

    @{
 */

#define DMAC_SAR0     (*((vuint32 *)(DMAC_BASE + 0x00)))
#define DMAC_SAR1     (*((vuint32 *)(DMAC_BASE + 0x10)))
#define DMAC_SAR2     (*((vuint32 *)(DMAC_BASE + 0x20)))
#define DMAC_SAR3     (*((vuint32 *)(DMAC_BASE + 0x30)))

/** @} */

/** \name  DMA Destination Address Registers (DAR0-DAR3)

    These registers designate the destination address for DMA transfers. 
    Currently we only support 32-byte boundary addresses.

    @{
 */

#define DMAC_DAR0     (*((vuint32 *)(DMAC_BASE + 0x04)))
#define DMAC_DAR1     (*((vuint32 *)(DMAC_BASE + 0x14)))
#define DMAC_DAR2     (*((vuint32 *)(DMAC_BASE + 0x24)))
#define DMAC_DAR3     (*((vuint32 *)(DMAC_BASE + 0x34)))

/** @} */

/** \name DMA Transfer Count Registers (DMATCR0-DMATCR3)

    These registers define the transfer count for each DMA channel. The count
    is defined as: num_bytes_to_transfer/32

    @{
 */

#define DMAC_DMATCR0  (*((vuint32 *)(DMAC_BASE + 0x08)))
#define DMAC_DMATCR1  (*((vuint32 *)(DMAC_BASE + 0x18)))
#define DMAC_DMATCR2  (*((vuint32 *)(DMAC_BASE + 0x28)))
#define DMAC_DMATCR3  (*((vuint32 *)(DMAC_BASE + 0x38)))

/** @} */

/** \name DMA Channel Control Registers (CHCR0-CHCR3)
  
    These registers configure the operating mode and transfer methodology for 
    each channel.

    For DMAC_CHCR2, it should always be set to 0x12c1 (source address 
    incremented, burst mode, interrupt disable, DMA enable).

    For DMAC_CHCR1 and DMAC_CHCR3, it would probably be set to 0x1241
    (source address incremented, cycle steal mode, interrupt disable,
    DMA enable).

    @{
 */

#define DMAC_CHCR0    (*((vuint32 *)(DMAC_BASE + 0x0c)))
#define DMAC_CHCR1    (*((vuint32 *)(DMAC_BASE + 0x1c)))
#define DMAC_CHCR2    (*((vuint32 *)(DMAC_BASE + 0x2c)))
#define DMAC_CHCR3    (*((vuint32 *)(DMAC_BASE + 0x3c)))

/** @} */


/** 
    \brief A register that dictates the overall operation of the DMAC.

    So far we only use it check the status of DMA operations.

 */
#define DMAC_DMAOR    (*((vuint32 *)(DMAC_BASE + 0x40)))

/** \name   List of helpful masks to check operations
 
    The DMAOR_STATUS_MASK captures the On-Demand Data Transfer Mode (Bit 15), 
    Address Error Flag (Bit 2), NMI Flag (Bit 1), and DMAC Master Enable (Bit 0).

    The DMAOR_NORMAL_OPERATION is a state where DMAC Master Enable is active, 
    and the On-Demand Data Transfer Mode is not set, with no address errors 
    or NMI inputs.

    @{
*/

#define DMAOR_STATUS_MASK       0x8007
#define DMAOR_NORMAL_OPERATION  0x8001

/** @} */

/** @} */

__END_DECLS

#endif  /* __DC_DMAC_H */

