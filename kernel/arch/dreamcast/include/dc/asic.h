/* KallistiOS ##version##

   dc/asic.h
   Copyright (C) 2001-2002 Megan Potter

*/

/** \file    dc/asic.h
    \brief   Dreamcast ASIC event handling support.
    \ingroup asic

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

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

/** \defgroup asic  Events
    \brief          Events pertaining to the DC's System ASIC
    \ingroup        system

*/

/* All event codes are two 8-bit integers; the top integer is the event code
   register to look in to check the event (and to acknowledge it). The
   register to check is 0xa05f6900+4*regnum. The bottom integer is the
   bit index within that register. */

/** \defgroup asic_events           Event Codes
    \brief                          Values for various Holly event codes
    \ingroup  asic
    @{
*/

/** \defgroup asic_events_pvr       PowerVR
    \brief                          Event code values for PowerVR events
    \ingroup  asic_events

    These are events that the PVR itself generates that can be hooked.
    @{
*/
#define ASIC_EVT_PVR_RENDERDONE_VIDEO     0x0000  /**< \brief Video render stage completed */
#define ASIC_EVT_PVR_RENDERDONE_ISP       0x0001  /**< \brief ISP render stage completed */
#define ASIC_EVT_PVR_RENDERDONE_TSP       0x0002  /**< \brief TSP render stage completed */
#define ASIC_EVT_PVR_VBLANK_BEGIN         0x0003  /**< \brief VBLANK begin interrupt */
#define ASIC_EVT_PVR_VBLANK_END           0x0004  /**< \brief VBLANK end interrupt */
#define ASIC_EVT_PVR_HBLANK_BEGIN         0x0005  /**< \brief HBLANK begin interrupt */

#define ASIC_EVT_PVR_YUV_DONE             0x0006  /**< \brief YUV completed */
#define ASIC_EVT_PVR_OPAQUEDONE           0x0007  /**< \brief Opaque list completed */
#define ASIC_EVT_PVR_OPAQUEMODDONE        0x0008  /**< \brief Opaque modifiers completed */
#define ASIC_EVT_PVR_TRANSDONE            0x0009  /**< \brief Transparent list completed */
#define ASIC_EVT_PVR_TRANSMODDONE         0x000a  /**< \brief Transparent modifiers completed */

#define ASIC_EVT_PVR_DMA                  0x0013  /**< \brief PVR DMA complete */
#define ASIC_EVT_PVR_PTDONE               0x0015  /**< \brief Punch-thrus completed */

#define ASIC_EVT_PVR_ISP_OUTOFMEM         0x0200  /**< \brief ISP out of memory */
#define ASIC_EVT_PVR_STRIP_HALT           0x0201  /**< \brief Halt due to strip buffer error */
#define ASIC_EVT_PVR_PARAM_OUTOFMEM       0x0202  /**< \brief Param out of memory */
#define ASIC_EVT_PVR_OPB_OUTOFMEM         0x0203  /**< \brief OPB went past PVR_TA_OPB_END */
#define ASIC_EVT_PVR_TA_INPUT_ERR         0x0204  /**< \brief Vertex input error */
#define ASIC_EVT_PVR_TA_INPUT_OVERFLOW    0x0205  /**< \brief Vertex input overflowed queue */
/** @} */

/** \defgroup asic_events_gd        GD-ROM Drive
    \brief                          Event code values for GD-ROM events
    \ingroup  asic_events

    These are events that the GD-ROM drive generates that can be hooked.
    @{
*/
#define ASIC_EVT_GD_COMMAND         0x0100  /**< \brief GD-Rom Command Status */
#define ASIC_EVT_GD_DMA             0x000e  /**< \brief GD-Rom DMA complete */
#define ASIC_EVT_GD_DMA_OVERRUN     0x020d  /**< \brief GD-Rom DMA overrun */
#define ASIC_EVT_GD_DMA_ILLADDR     0x020c  /**< \brief GD-Rom DMA illegal address */
/** @} */

/** \defgroup asic_events_maple     Maple
    \brief                          Event code values for Maple events
    \ingroup  asic_events

    These are events that Maple generates that can be hooked.
    @{
*/
#define ASIC_EVT_MAPLE_DMA          0x000c  /**< \brief Maple DMA complete */
#define ASIC_EVT_MAPLE_ERROR        0x000d  /**< \brief Maple error (?) */
/** @} */

/** \defgroup asic_events_spu       AICA
    \brief                          Event code values for AICA events
    \ingroup  asic_events

    These are events that the SPU (AICA) generates that can be hooked.
    @{
*/
#define ASIC_EVT_SPU_DMA            0x000f  /**< \brief SPU (G2 channel 0) DMA complete */
#define ASIC_EVT_SPU_IRQ            0x0101  /**< \brief SPU interrupt */
/** @} */

/** \defgroup asic_events_g2dma     G2 Bus DMA
    \brief                          Event code values for G2 Bus events
    \ingroup  asic_events

    These are events that G2 bus DMA generates that can be hooked.
    @{
*/
#define ASIC_EVT_G2_DMA0            0x000f  /**< \brief G2 DMA channel 0 complete */
#define ASIC_EVT_G2_DMA1            0x0010  /**< \brief G2 DMA channel 1 complete */
#define ASIC_EVT_G2_DMA2            0x0011  /**< \brief G2 DMA channel 2 complete */
#define ASIC_EVT_G2_DMA3            0x0012  /**< \brief G2 DMA channel 3 complete */
/** @} */

/** \defgroup asic_events_ext      External Port
    \brief                          Event code values for external port events
    \ingroup  asic_events

    These are events that external devices generate that can be hooked.
    @{
*/
#define ASIC_EVT_EXP_8BIT           0x0102  /**< \brief Modem / Lan Adapter */
#define ASIC_EVT_EXP_PCI            0x0103  /**< \brief BBA IRQ */
/** @} */

/** @} */

/** \defgroup asic_regs             Registers
    \brief                          Addresses for various ASIC eveng registers
    \ingroup  asic

    These are the locations in memory where the ASIC registers sit.
    @{
*/
#define ASIC_ACK_A            0xa05f6900  /**< \brief IRQD ACK register */
#define ASIC_ACK_B            0xa05f6904  /**< \brief IRQB ACK register */
#define ASIC_ACK_C            0xa05f6908  /**< \brief IRQ9 ACK register */

#define ASIC_IRQD_A            0xa05f6910  /**< \brief IRQD first register */
#define ASIC_IRQD_B            0xa05f6914  /**< \brief IRQD second register */
#define ASIC_IRQD_C            0xa05f6918  /**< \brief IRQD third register */
#define ASIC_IRQB_A            0xa05f6920  /**< \brief IRQB first register */
#define ASIC_IRQB_B            0xa05f6924  /**< \brief IRQB second register */
#define ASIC_IRQB_C            0xa05f6928  /**< \brief IRQB third register */
#define ASIC_IRQ9_A            0xa05f6930  /**< \brief IRQ9 first register */
#define ASIC_IRQ9_B            0xa05f6934  /**< \brief IRQ9 second register */
#define ASIC_IRQ9_C            0xa05f6938  /**< \brief IRQ9 third register */
/** @} */

/** \defgroup asic_irq_lv           IRQ Levels
    \brief                          values for the various ASIC event IRQ levels
    \ingroup  asic

    You can pick one at hook time, or don't choose anything and the default will
    be used instead.
    @{
*/
#define ASIC_IRQ9           0  /**< \brief IRQ level 9 */
#define ASIC_IRQB           1  /**< \brief IRQ level B (11) */
#define ASIC_IRQD           2  /**< \brief IRQ level D (13) */

#define ASIC_IRQ_MAX        3  /**< \brief Don't take irqs from here up */
#define ASIC_IRQ_DEFAULT    ASIC_IRQ9  /**< \brief Pick an IRQ level for me! */
/** @} */

/** \brief   ASIC event handler type.
    \ingroup asic

    Any event handlers registered must be of this type. These will be run in an
    interrupt context, so don't try anything funny.

    \param  code            The ASIC event code that generated this event.
    \param  data            The user pointer that was passed to
                            \ref asic_evt_set_handler.
    \see    asic_events
*/
typedef void (*asic_evt_handler)(uint32_t code, void *data);

/** \brief   ASIC event handler table entry.
    \ingroup asic
 */
typedef struct {
    asic_evt_handler hdl;
    void *data;
} asic_evt_handler_entry_t;

/** \brief   Set or remove an ASIC handler.
    \ingroup asic

    This function will register an event handler for a given event code, or if
    the handler is NULL, unregister any that is currently registered.

    \param  code            The ASIC event code to hook (see \ref asic_events).
    \param  handler         The function to call when the event happens.
    \param  data            A user pointer that will be passed to the callback.

*/
asic_evt_handler_entry_t asic_evt_set_handler(uint16_t code, asic_evt_handler handler, void *data);

/** \brief   Register a threaded handler with the given ASIC event.
    \ingroup asic

    This function will spawn a thread, that will sleep until notified when an
    event happens. It will then call the handler. As the handler is not called
    in an interrupt context, it can hold locks, and even sleep.

    \param  code            The ASIC event code to hook (see \ref asic_events).
    \param  handler         The function to call when the event happens.
    \param  data            A user pointer that will be passed to the callback.
    \param  ack_and_mask    An optional function that will be called by the real
                            interrupt handler, to acknowledge and mask the
                            interrupt, so that it won't trigger again while the
                            threaded handler is running.
    \param  unmask          An optional function that will be called by the
                            thread after the handler function returned, to
                            re-enable the interrupt.
*/
int asic_evt_request_threaded_handler(uint16_t code, asic_evt_handler handler,
                                      void *data,
                                      void (*ack_and_mask)(uint16_t),
                                      void (*unmask)(uint16_t));

/** \brief   Unregister any handler set to the given ASIC event.
    \ingroup asic

    \param  code            The ASIC event code to unhook (see
                            \ref asic_events).
*/
void asic_evt_remove_handler(uint16_t code);

/** \brief   Disable all ASIC events.
    \ingroup asic

    This function will disable hooks for every event that has been hooked. In
    order to reinstate them, you must individually re-enable them. Not a very
    good idea to be doing this normally.
*/
void asic_evt_disable_all(void);

/** \brief   Disable one ASIC event.
    \ingroup asic

    This function will disable the hook for a specified code that was registered
    at the given IRQ level. Generally, you will never have to do this yourself
    unless you're adding in some new functionality.

    \param  code            The ASIC event code to unhook (see
                            \ref asic_events).
    \param  irqlevel        The IRQ level it was hooked on (see
                            \ref asic_irq_lv).
*/
void asic_evt_disable(uint16_t code, uint8_t irqlevel);

/** \brief   Enable an ASIC event.
    \ingroup asic

    This function will enable the hook for a specified code and register it at
    the given IRQ level. You should only register each event at a max of one
    IRQ level (this will not check that for you), and this does not actually set
    the hook function for the event, you must do that separately with
    asic_evt_set_handler(). Generally, unless you're adding in new
    functionality, you'll never have to do this.

    \param  code            The ASIC event code to hook (see \ref asic_events).
    \param  irqlevel        The IRQ level to hook on (see \ref asic_irq_lv).
 */
void asic_evt_enable(uint16_t code, uint8_t irqlevel);

/** \cond   Enable ASIC events */
void asic_init(void);
/* Shutdown ASIC events, disabling all hooks. */
void asic_shutdown(void);
/** \endcond */

__END_DECLS

#endif  /* __DC_ASIC_H */
