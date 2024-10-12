/* KallistiOS ##version##

   arch/dreamcast/include/irq.h
   Copyright (C) 2000-2001 Megan Potter
   Copyright (C) 2024 Paul Cercueil
   Copyright (C) 2024 Falco Girgis

*/

/** \file
    \brief   Interrupt and exception handling.
    \ingroup irqs

    This file contains various definitions and declarations related to handling
    interrupts and exceptions on the Dreamcast. This level deals with IRQs and
    exceptions generated on the SH4, versus the asic layer which deals with
    actually differentiating "external" interrupts.

    \author Megan Potter
    \author Paul Cercueil
    \author Falco Girgis

    \see    dc/asic.h, arch/trap.h
*/

#ifndef __ARCH_IRQ_H
#define __ARCH_IRQ_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/* Included for legacy compatibility with these two APIs being one. */
#include <arch/trap.h>

/** \defgroup irqs  Interrupts
    \brief          IRQs and ISRs for the SH4's CPU
    \ingroup        system

    This is an API for managing interrupts, their masks, and their
    handler routines along with thread context information.

    \warning
    This is a low-level, internal kernel API. Many of these
    interrupts are utilized by various KOS drivers and have higher-level APIs
    for hooking into them. Care must be taken to not interfere with the IRQ
    handling which is being done by in-use KOS drivers.

    @{
*/

/** \defgroup Context
    \brief Thread execution state and accessors

    This API includes the structure and accessors for a
    thread's context state, which contains the registers that are stored
    and loaded upon thread context switches, which are passed back to
    interrupt handlers.

    @{
*/

/** The number of bytes required to save thread context.

    This should include all general CPU registers, FP registers, and status regs
    (even if not all of these are actually used).

    \note
    On the Dreamcast, we need `228` bytes for all of that, but we round it up to a
    nicer number for sanity.
*/
#define REG_BYTE_CNT 256

/** Architecture-specific structure for holding the processor state.

    This structure should hold register values and other important parts of the
    processor state.

    \note
    The size of this structure should be less than or equal to the
    \ref REG_BYTE_CNT value.
*/
typedef __attribute__((aligned(32))) struct irq_context {
    uint32_t  pc;         /**< Program counter */
    uint32_t  pr;         /**< Procedure register (aka return address) */
    uint32_t  gbr;        /**< Global base register (TLS segment ptr) */
    uint32_t  vbr;        /**< Vector base register */
    uint32_t  mach;       /**< Multiply-and-accumulate register (high) */
    uint32_t  macl;       /**< Multiply-and-accumulate register (low) */
    uint32_t  sr;         /**< Status register */
    uint32_t  fpul;       /**< Floating-point communication register */
    uint32_t  fr[16];     /**< Primary floating point registers */
    uint32_t  frbank[16]; /**< Secondary floating point registers */
    uint32_t  r[16];      /**< 16 general purpose (integer) registers */
    uint32_t  fpscr;      /**< Floating-point status/control register */
} irq_context_t;

/** \name Register Accessors
    \brief Convenience macros for accessing context registers
    @{
*/
/** Fetch the program counter from an irq_context_t.
    \param  c               The context to read from.
    \return                 The program counter value.
*/
#define CONTEXT_PC(c)   ((c).pc)

/** Fetch the frame pointer from an irq_context_t.
    \param  c               The context to read from.
    \return                 The frame pointer value.
*/
#define CONTEXT_FP(c)   ((c).r[14])

/** Fetch the stack pointer from an irq_context_t.
    \param  c               The context to read from.
    \return                 The stack pointer value.
*/
#define CONTEXT_SP(c)   ((c).r[15])

/** Fetch the return value from an irq_context_t.
    \param  c               The context to read from.
    \return                 The return value.
*/
#define CONTEXT_RET(c)  ((c).r[0])
/** @} */

/** Switch out contexts (for interrupt return).

    This function will set the processor state that will be restored when the
    exception returns.

    \param  regbank         The values of all registers to be restored.

    \sa irq_get_context()
*/
void irq_set_context(irq_context_t *regbank);

/** Get the current IRQ context.

    This will fetch the processor context prior to the exception handling during
    an IRQ service routine.

    \return                 The current IRQ context.

    \sa irq_set_context()
*/
irq_context_t *irq_get_context(void);

/** Fill a newly allocated context block.

    The given parameters will be passed to the called routine (up to the
    architecture maximum). For the Dreamcast, this maximum is 4.

    \param  context         The IRQ context to fill in.
    \param  stack_pointer   The value to set in the stack pointer.
    \param  routine         The address of the program counter for the context.
    \param  args            Any arguments to set in the registers. This cannot
                            be NULL, and must have enough values to fill in up
                            to the architecture maximum.
    \param  usermode        true to run the routine in user mode, false for
                            supervisor.
*/
void irq_create_context(irq_context_t *context, uint32_t stack_pointer,
                        uint32_t routine, const uint32_t *args, bool usermode);

/** @} */

/** Interrupt exception codes

   Dreamcast-specific exception codes. Used to identify the source or type of
   an interrupt. Each exception code is of a certain "type" which dictates how the interrupt
   is generated and handled.

    List of exception types:

   |Type    | Description
   |--------|------------
   |`RESET` | Caused by system reset. Uncatchable and fatal. Automatically branch to address `0xA0000000`.
   |`REEXEC`| Restarts current instruction after interrupt processing. Context PC is the triggering instruction.
   |`POST`  | Continues with next instruciton after interrupt processing. Context PC is the next instruction.
   |`SOFT`  | Software-driven exceptions for triggering interrupts upon special events.
   |`UNUSED`| Known to not be present and usable with the DC's SH4 configuration.

    List of exception codes:
*/
typedef enum irq_exception {
    EXC_RESET_POWERON      = 0x0000, /**< `[RESET ]` Power-on reset */
    EXC_RESET_MANUAL       = 0x0020, /**< `[RESET ]` Manual reset */
    EXC_RESET_UDI          = 0x0000, /**< `[RESET ]` Hitachi UDI reset */
    EXC_ITLB_MULTIPLE      = 0x0140, /**< `[RESET ]` Instruction TLB multiple hit */
    EXC_DTLB_MULTIPLE      = 0x0140, /**< `[RESET ]` Data TLB multiple hit */
    EXC_USER_BREAK_PRE     = 0x01e0, /**< `[REEXEC]` User break before instruction */
    EXC_INSTR_ADDRESS      = 0x00e0, /**< `[REEXEC]` Instruction address */
    EXC_ITLB_MISS          = 0x0040, /**< `[REEXEC]` Instruction TLB miss */
    EXC_ITLB_PV            = 0x00a0, /**< `[REEXEC]` Instruction TLB protection violation */
    EXC_ILLEGAL_INSTR      = 0x0180, /**< `[REEXEC]` Illegal instruction */
    EXC_SLOT_ILLEGAL_INSTR = 0x01a0, /**< `[REEXEC]` Slot illegal instruction */
    EXC_GENERAL_FPU        = 0x0800, /**< `[REEXEC]` General FPU exception */
    EXC_SLOT_FPU           = 0x0820, /**< `[REEXEC]` Slot FPU exception */
    EXC_DATA_ADDRESS_READ  = 0x00e0, /**< `[REEXEC]` Data address (read) */
    EXC_DATA_ADDRESS_WRITE = 0x0100, /**< `[REEXEC]` Data address (write) */
    EXC_DTLB_MISS_READ     = 0x0040, /**< `[REEXEC]` Data TLB miss (read) */
    EXC_DTLB_MISS_WRITE    = 0x0060, /**< `[REEXEC]` Data TLB miss (write) */
    EXC_DTLB_PV_READ       = 0x00a0, /**< `[REEXEC]` Data TLB protection violation (read) */
    EXC_DTLB_PV_WRITE      = 0x00c0, /**< `[REEXEC]` Data TLB protection violation (write) */
    EXC_FPU                = 0x0120, /**< `[REEXEC]` FPU exception */
    EXC_INITIAL_PAGE_WRITE = 0x0080, /**< `[REEXEC]` Initial page write exception */
    EXC_TRAPA              = 0x0160, /**< `[POST  ]` Unconditional trap (`TRAPA`) */
    EXC_USER_BREAK_POST    = 0x01e0, /**< `[POST  ]` User break after instruction */
    EXC_NMI                = 0x01c0, /**< `[POST  ]` Nonmaskable interrupt */
    EXC_IRQ0               = 0x0200, /**< `[POST  ]` External IRQ request (level 0) */
    EXC_IRQ1               = 0x0220, /**< `[POST  ]` External IRQ request (level 1) */
    EXC_IRQ2               = 0x0240, /**< `[POST  ]` External IRQ request (level 2) */
    EXC_IRQ3               = 0x0260, /**< `[POST  ]` External IRQ request (level 3) */
    EXC_IRQ4               = 0x0280, /**< `[POST  ]` External IRQ request (level 4) */
    EXC_IRQ5               = 0x02a0, /**< `[POST  ]` External IRQ request (level 5) */
    EXC_IRQ6               = 0x02c0, /**< `[POST  ]` External IRQ request (level 6) */
    EXC_IRQ7               = 0x02e0, /**< `[POST  ]` External IRQ request (level 7) */
    EXC_IRQ8               = 0x0300, /**< `[POST  ]` External IRQ request (level 8) */
    EXC_IRQ9               = 0x0320, /**< `[POST  ]` External IRQ request (level 9) */
    EXC_IRQA               = 0x0340, /**< `[POST  ]` External IRQ request (level 10) */
    EXC_IRQB               = 0x0360, /**< `[POST  ]` External IRQ request (level 11) */
    EXC_IRQC               = 0x0380, /**< `[POST  ]` External IRQ request (level 12) */
    EXC_IRQD               = 0x03a0, /**< `[POST  ]` External IRQ request (level 13) */
    EXC_IRQE               = 0x03c0, /**< `[POST  ]` External IRQ request (level 14) */
    EXC_TMU0_TUNI0         = 0x0400, /**< `[POST  ]` TMU0 underflow */
    EXC_TMU1_TUNI1         = 0x0420, /**< `[POST  ]` TMU1 underflow */
    EXC_TMU2_TUNI2         = 0x0440, /**< `[POST  ]` TMU2 underflow */
    EXC_TMU2_TICPI2        = 0x0460, /**< `[UNUSED]` TMU2 input capture */
    EXC_RTC_ATI            = 0x0480, /**< `[UNUSED]` RTC alarm interrupt */
    EXC_RTC_PRI            = 0x04a0, /**< `[UNUSED]` RTC periodic interrupt */
    EXC_RTC_CUI            = 0x04c0, /**< `[UNUSED]` RTC carry interrupt */
    EXC_SCI_ERI            = 0x04e0, /**< `[UNUSED]` SCI Error receive */
    EXC_SCI_RXI            = 0x0500, /**< `[UNUSED]` SCI Receive ready */
    EXC_SCI_TXI            = 0x0520, /**< `[UNUSED]` SCI Transmit ready */
    EXC_SCI_TEI            = 0x0540, /**< `[UNUSED]` SCI Transmit error */
    EXC_WDT_ITI            = 0x0560, /**< `[POST  ]` Watchdog timer */
    EXC_REF_RCMI           = 0x0580, /**< `[POST  ]` Memory refresh compare-match interrupt */
    EXC_REF_ROVI           = 0x05a0, /**< `[POST  ]` Memory refresh counter overflow interrupt */
    EXC_UDI                = 0x0600, /**< `[POST  ]` Hitachi UDI */
    EXC_GPIO_GPIOI         = 0x0620, /**< `[POST  ]` I/O port interrupt */
    EXC_DMAC_DMTE0         = 0x0640, /**< `[POST  ]` DMAC transfer end (channel 0) */
    EXC_DMAC_DMTE1         = 0x0660, /**< `[POST  ]` DMAC transfer end (channel 1) */
    EXC_DMAC_DMTE2         = 0x0680, /**< `[POST  ]` DMAC transfer end (channel 2) */
    EXC_DMAC_DMTE3         = 0x06a0, /**< `[POST  ]` DMAC transfer end (channel 3) */
    EXC_DMA_DMAE           = 0x06c0, /**< `[POST  ]` DMAC address error */
    EXC_SCIF_ERI           = 0x0700, /**< `[POST  ]` SCIF Error receive */
    EXC_SCIF_RXI           = 0x0720, /**< `[POST  ]` SCIF Receive ready */
    EXC_SCIF_BRI           = 0x0740, /**< `[POST  ]` SCIF break */
    EXC_SCIF_TXI           = 0x0760, /**< `[POST  ]` SCIF Transmit ready */
    EXC_DOUBLE_FAULT       = 0x0ff0, /**< `[SOFT  ]` Exception happened in an ISR */
    EXC_UNHANDLED_EXC      = 0x0fe0  /**< `[SOFT  ]` Exception went unhandled */
} irq_t;


/** \defgroup  irq_type_offsets        Exception type offsets
    \brief                             Offsets within exception types
    \ingroup                           irqs

    The following are a table of "type offsets" (see the Hitachi PDF). These are
    the 0x000, 0x100, 0x400, and 0x600 offsets.

    @{
*/
#define EXC_OFFSET_000  0   /**< \brief Offset 0x000 */
#define EXC_OFFSET_100  1   /**< \brief Offset 0x100 */
#define EXC_OFFSET_400  2   /**< \brief Offset 0x400 */
#define EXC_OFFSET_600  3   /**< \brief Offset 0x600 */
/** @} */

/** \brief   The value of the timer IRQ
    \ingroup irqs
*/
#define TIMER_IRQ       EXC_TMU0_TUNI0

/** \defgroup irq_state     State
    \brief                  Methods for querying active IRQ information.

    Provides an API for accessing the state of the current IRQ context such
    as the active interrupt or whether it has been handled.

    @{
*/


/** Returns whether inside of an interrupt context.

    \retval non-zero        If interrupt handling is in progress.
                            ((code&0xf)<<16) | (evt&0xffff)
    \retval 0               If normal processing is in progress.

*/
int irq_inside_int(void);

/** @} */

/** \defgroup irq_mask      Mask
    \brief                  Accessors and modifiers of the IMASK state.

    This API is provided for managing and querying information regarding the
    interrupt mask, a series of bitflags representing whether each type of
    interrupt has been enabled or not.

    @{
*/

/** Type representing an interrupt mask state. */
typedef uint32_t irq_mask_t;
/** Disable interrupts.

    This function will disable interrupts, but will leave exceptions enabled.

    \return                 The state of IRQs before calling the function. This
                            can be used to restore this state later on with
                            irq_restore().

    \sa irq_restore(), irq_enable()
*/
irq_mask_t irq_disable(void);

/** Enable all interrupts.

    This function will enable ALL interrupts, including external ones.

    \sa irq_disable()
*/
void irq_enable(void);

/** Restore IRQ state.

    This function will restore the interrupt state to the value specified. This
    should correspond to a value returned by irq_disable().

    \param  v               The IRQ state to restore. This should be a value
                            returned by irq_disable().

    \sa irq_disable()
*/
void irq_restore(irq_mask_t v);

/** @} */

/** \defgroup irq_ctrl Control Flow
    \brief Methods for managing control flow within an irq_handler.

    This API provides methods for controlling program flow from within an
    active interrupt handler.

    @{
*/

/** Resume normal execution from IRQ context.

    Pretend like we just came in from an interrupt and force a context switch
    back to the "current" context.

    \warning
    Make sure you've called irq_set_context() before doing this!

    \sa irq_set_context()
*/
void irq_force_return(void);

/** @} */

/** \defgroup irq_handlers  Handlers
    \brief                  API for managing IRQ handlers

    This API provides a series of methods for registering and retrieving
    different types of exception handlers.

    @{
*/

/** The type of an IRQ handler.

    \param  code            The IRQ that caused the handler to be called.
    \param  context         The CPU's context.
    \param  data            Arbitrary userdata associated with the handler.
*/
typedef void (*irq_handler)(irq_t code, irq_context_t *context, void *data);

/** \defgroup irq_handlers_ind  Individual
    \brief                      API for managing individual IRQ handlers.

    This API is for managing handlers installed to handle individual IRQ codes.

    @{
*/

/** Set or remove an IRQ handler.
    
    Passing a NULL value for hnd will remove the current handler, if any.

    \param  code            The IRQ type to set the handler for
                            (see \ref irq_exception_codes).
    \param  hnd             A pointer to a procedure to handle the exception.
    \param  data            A pointer that will be passed along to the callback.
    
    \retval 0               On success.
    \retval -1              If the code is invalid.

    \sa irq_get_handler()
*/
int irq_set_handler(irq_t code, irq_handler hnd, void *data);

/** Get the address of the current handler for the IRQ type.

    \param  code            The IRQ type to look up.
    
    \return                 A pointer to the procedure to handle the exception.

    \sa irq_set_handler()
*/
irq_handler irq_get_handler(irq_t code);

/** @} */

/** \defgroup irq_handlers_global   Global
    \brief                          API for managing global IRQ handler.

    @{
*/
/** Set a global exception handler.

    This function sets a global catch-all filter for all exception types.

    \note                   The specific handler will still be called for the
                            exception if one is set. If not, setting one of
                            these will stop the unhandled exception error.

    \param  hnd             A pointer to the procedure to handle the exception.
    \param  data            A pointer that will be passed along to the callback.

    \retval 0               On success (no error conditions defined).

*/
int irq_set_global_handler(irq_handler handler, void *data);

/** Get the global exception handler.

    \return                 The global exception handler set with
                            irq_set_global_handler(), or NULL if none is set.
*/
irq_handler irq_get_global_handler(void);
/** @} */

/** @} */

/** \cond INTERNAL */

/** Initialize interrupts.

    \retval 0               On success (no error conditions defined).

    \sa irq_shutdown()
*/
int irq_init(void);

/** Shutdown interrupts.

    Restores the state to how it was before irq_init() was called.

    \sa irq_init()
*/
void irq_shutdown(void);

static inline void __irq_scoped_cleanup(int *state) {
    irq_restore(*state);
}

#define ___irq_disable_scoped(l) \
    int __scoped_irq_##l __attribute__((cleanup(__irq_scoped_cleanup))) = irq_disable()

#define __irq_disable_scoped(l) ___irq_disable_scoped(l)
/** \endcond */

/** \brief  Disable interrupts with scope management.

    This macro will disable interrupts, similarly to irq_disable(), with the
    difference that the interrupt state will automatically be restored once the
    execution exits the functional block in which the macro was called.
*/
#define irq_disable_scoped() __irq_disable_scoped(__LINE__)

/** @} */

__END_DECLS

#endif  /* __ARCH_IRQ_H */