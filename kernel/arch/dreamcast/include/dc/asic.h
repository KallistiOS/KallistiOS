/* KallistiOS ##version##

   dc/asic.h
   Copyright (C) 2001-2002 Megan Potter

*/

/** \file   dc/asic.h
    \brief  Dreamcast ASIC event handling support.

    This file provides definitions of the events that the ASIC (a part of the
    PVR) in the Dreamcast can trigger as IRQs, and ways to set responders for
    those events. Pretty much, this covers all IRQs that aren't generated
    internally in the SH4 (SCIF and the SH4 DMAC can generate their own IRQs,
    as well as the trapa instruction, and various exceptions -- those are not
    dealt with here).

    \author Megan Potter
*/

#ifndef __DC_ASIC_H
#define __DC_ASIC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/* All event codes are two 8-bit integers; the top integer is the event code
   register to look in to check the event (and to acknolwedge it). The
   register to check is 0xa05f6900+4*regnum. The bottom integer is the
   bit index within that register. */

/** \defgroup asic_events           ASIC event codes
    @{
*/

/** \defgroup asic_pvr_evts         Event codes for the PVR chip

    These are events that the PVR itself generates that can be hooked.
    @{
*/
#define ASIC_EVT_PVR_RENDERDONE     0x0002  /**< \brief Render completed */
#define ASIC_EVT_PVR_SCANINT1       0x0003  /**< \brief Scanline interrupt 1 */
#define ASIC_EVT_PVR_SCANINT2       0x0004  /**< \brief Scanline interrupt 2 */
#define ASIC_EVT_PVR_VBLINT         0x0005  /**< \brief VBL interrupt */
#define ASIC_EVT_PVR_OPAQUEDONE     0x0007  /**< \brief Opaque list completed */
#define ASIC_EVT_PVR_OPAQUEMODDONE  0x0008  /**< \brief Opaque modifiers completed */
#define ASIC_EVT_PVR_TRANSDONE      0x0009  /**< \brief Transparent list completed */
#define ASIC_EVT_PVR_TRANSMODDONE   0x000a  /**< \brief Transparent modifiers completed */
#define ASIC_EVT_PVR_DMA            0x0013  /**< \brief PVR DMA complete */
#define ASIC_EVT_PVR_PTDONE         0x0015  /**< \brief Punch-thrus completed */
#define ASIC_EVT_PVR_PRIMOUTOFMEM   0x0202  /**< \brief Out of primitive memory */
#define ASIC_EVT_PVR_MATOUTOFMEM    0x0203  /**< \brief Out of matrix memory */
/** @} */

/** \defgroup asic_gd_evts          Event codes for the GD controller

    These are events that the GD-ROM drive generates that can be hooked.
    @{
*/
#define ASIC_EVT_GD_COMMAND         0x0100  /**< \brief GD-Rom Command Status */
#define ASIC_EVT_GD_DMA             0x000e  /**< \brief GD-Rom DMA complete */
#define ASIC_EVT_GD_DMA_OVERRUN     0x020d  /**< \brief GD-Rom DMA overrun */
#define ASIC_EVT_GD_DMA_ILLADDR     0x020c  /**< \brief GD-Rom DMA illegal address */
/** @} */

/** \defgroup asic_maple_evts       Event codes for the Maple controller

    These are events that Maple generates that can be hooked.
    @{
*/
#define ASIC_EVT_MAPLE_DMA          0x000c  /**< \brief Maple DMA complete */
#define ASIC_EVT_MAPLE_ERROR        0x000d  /**< \brief Maple error (?) */
/** @} */

/** \defgroup asic_spu_evts         Event codes for the SPU

    These are events that the SPU (AICA) generates that can be hooked.
    @{
*/
#define ASIC_EVT_SPU_DMA            0x000f  /**< \brief SPU (G2 channel 0) DMA complete */
#define ASIC_EVT_SPU_IRQ            0x0101  /**< \brief SPU interrupt */
/** @} */

/** \defgroup asic_g2dma_evts       Event codes for G2 bus DMA

    These are events that G2 bus DMA generates that can be hooked.
    @{
*/
#define ASIC_EVT_G2_DMA0            0x000f  /**< \brief G2 DMA channel 0 complete */
#define ASIC_EVT_G2_DMA1            0x0010  /**< \brief G2 DMA channel 1 complete */
#define ASIC_EVT_G2_DMA2            0x0011  /**< \brief G2 DMA channel 2 complete */
#define ASIC_EVT_G2_DMA3            0x0012  /**< \brief G2 DMA channel 3 complete */
/** @} */

/** \defgroup asic_ext_evts         Event codes for the external port

    These are events that external devices generate that can be hooked.
    @{
*/
#define ASIC_EVT_EXP_8BIT           0x0102  /**< \brief Modem / Lan Adapter */
#define ASIC_EVT_EXP_PCI            0x0103  /**< \brief BBA IRQ */
/** @}
    @} */

/** \defgroup asic_regs             ASIC registers

    These are the locations in memory where the ASIC registers sit.
    @{
*/
#define ASIC_IRQD_A (*(vuint32*)0xa05f6910) /**< \brief IRQD first register */
#define ASIC_IRQD_B (*(vuint32*)0xa05f6914) /**< \brief IRQD second register */
#define ASIC_IRQD_C (*(vuint32*)0xa05f6918) /**< \brief IRQD third register */
#define ASIC_IRQB_A (*(vuint32*)0xa05f6920) /**< \brief IRQB first register */
#define ASIC_IRQB_B (*(vuint32*)0xa05f6924) /**< \brief IRQB second register */
#define ASIC_IRQB_C (*(vuint32*)0xa05f6928) /**< \brief IRQB third register */
#define ASIC_IRQ9_A (*(vuint32*)0xa05f6930) /**< \brief IRQ9 first register */
#define ASIC_IRQ9_B (*(vuint32*)0xa05f6934) /**< \brief IRQ9 second register */
#define ASIC_IRQ9_C (*(vuint32*)0xa05f6938) /**< \brief IRQ9 third register */

#define ASIC_ACK_A  (*(vuint32*)0xa05f6900) /**< \brief IRQD ACK register */
#define ASIC_ACK_B  (*(vuint32*)0xa05f6904) /**< \brief IRQB ACK register */
#define ASIC_ACK_C  (*(vuint32*)0xa05f6908) /**< \brief IRQ9 ACK register */
/** @} */

/** \defgroup asic_irq_lv           ASIC IRQ levels

    You can pick one at hook time, or don't choose anything and the default will
    be used instead.
    @{
*/
#define ASIC_IRQ9           1   /**< \brief IRQ level 9 */
#define ASIC_IRQB           2   /**< \brief IRQ level B (11) */
#define ASIC_IRQD           3   /**< \brief IRQ level D (13) */
#define ASIC_IRQ_DEFAULT    0   /**< \brief Pick an IRQ level for me! */
/** @} */

/** \brief  ASIC event handler type.

    Any event handlers registered must be of this type. These will be run in an
    interrupt context, so don't try anything funny.

    \param  code            The ASIC event code that generated this event.
    \see    asic_events
*/
typedef void (*asic_evt_handler)(uint32 code);

/** \brief  Set or remove an ASIC handler.

    This function will register an event handler for a given event code, or if
    the handler is NULL, unregister any that is currently registered.

    \param  code            The ASIC event code to hook (see \ref asic_events).
    \param  handler         The function to call when the event happens.
    \retval 0               On success.
    \retval -1              If an invalid code is specified.
*/
int asic_evt_set_handler(uint32 code, asic_evt_handler handler);

/** \brief  Disable all ASIC events.

    This function will disable hooks for every event that has been hooked. In
    order to reinstate them, you must individually re-enable them. Not a very
    good idea to be doing this normally.
*/
void asic_evt_disable_all();

/** \brief  Disable one ASIC event.

    This function will disable the hook for a specified code that was registered
    at the given IRQ level. Generally, you will never have to do this yourself
    unless you're adding in some new functionality.

    \param  code            The ASIC event code to unhook (see
                            \ref asic_events).
    \param  irqlevel        The IRQ level it was hooked on (see
                            \ref asic_irq_lv).
*/
void asic_evt_disable(uint32 code, int irqlevel);

/** \brief  Enable an ASIC event.

    This function will enable the hook for a specified code and register it at
    the given IRQ level. You should only register each event at a max of one
    IRQ level (this will not check that for you), and this does not actually set
    the hook function for the event, you must do that separately with
    asic_evt_set_handler(). Generally, unless you're adding in new
    functionality, you'll never have to do this.

    \param  code            The ASIC event code to hook (see \ref asic_events).
    \param  irqlevel        The IRQ level to hook on (see \ref asic_irq_lv).
 */
void asic_evt_enable(uint32 code, int irqlevel);

/** \brief  Init ASIC events. */
void asic_init();

/** \brief  Shutdown ASIC events, disabling all hooks. */
void asic_shutdown();

__END_DECLS

#endif  /* __DC_ASIC_H */
