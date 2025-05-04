/* KallistiOS ##version##

   arch/dreamcast/include/irq.h
   Copyright (C) 2000-2001 Megan Potter
   Copyright (C) 2024 Paul Cercueil
   Copyright (C) 2024, 2025 Falco Girgis

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

    \todo
        - Save and restore SH4 DBR register in irq_context_t.
        - Add support for `EXPMASK` (assuming DC's SH4 supports it).
        - Stop including trap.h from irq.h.
        - Explicitly label unimplemented interrupts for DC with separate type
        - Truly support nested interrupts and exceptions
            - irq_context_t needs to not be static, ASM adjustment.
*/

#ifndef __ARCH_IRQ_H
#define __ARCH_IRQ_H

#include <stdalign.h>
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

    ## IRQ Handler Propagation
    Interrupt processing routes an interrupt request through a configurable
    chain of handlers, propagating the requeset down the chain. See \ref
    irq_handlers for more details.

    ## Nested IRQ Handlers
    While the mechanism is in place for the future, KOS currently only
    gracefully handles one level of interrupts or a maximum depth of 1.
    The exception to this rule is when another one fires, and the
    `EXC_DOUBLE_FAULT` handler is called. From within this handler,
    there will be a depth of 2 right before the system must terminate,
    as there is no graceful way to recover from this state.

    \warning
    This is a low-level, internal kernel API. Many of these
    interrupts are utilized by various KOS drivers and have higher-level APIs
    for hooking into them. Care must be taken to not interfere with the IRQ
    handling which is being done by in-use KOS drivers. It is often better to
    install a global filter to observe interrupt events passively than it is
    to override individual interrupt handlers which have been used by the
    system.

    \note
    The naming convention used by this API differs from that of the actual SH4
    manual for historical reasons (it wasn't platform-specific). The SH4 manual
    refers to the most general type of CPU events which result in a SW
    callback as "exceptions," with "interrupts" and "general exceptions" being
    subtypes of exceptions. This API uses the term "interrupt" and "exception"
    interchangeably, except where it is explicitly noted that "SH4 interrupts"
    or "SH4 general exceptions" are being referred to, more specifically.

    \note
    The naming convention used by this API differs from that of the actual SH4
    manual for historical reasons (it wasn't platform-specific). The SH4 manual
    refers to the most general type of CPU events which result in a SW
    callback as "exceptions," with "interrupts" and "general exceptions" being
    subtypes of exceptions. This API uses the term "interrupt" and "exception"
    interchangeably, except where it is explicitly noted that "SH4 interrupts"
    or "SH4 general exceptions" are being referred to, more specifically.

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
typedef struct irq_context {
alignas(32)
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

   SH-specific exception codes. Used to identify the source or type of an
   interrupt. Each exception code is of a certain "type" which dictates how the
   interrupt is generated and handled.

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
    EXC_DOUBLE_FAULT       = 0x0780, /**< `[SOFT  ]` Exception happened in an ISR */
    EXC_UNHANDLED_EXC      = 0x07e0  /**< `[SOFT  ]` Exception went unhandled */
} irq_t;

/** \defgroup irq_state     State
    \brief                  Methods for querying active IRQ information.

    Provides an API for accessing the state of the current IRQ context such
    as the active interrupt or whether it has been handled.

    @{
*/

/** Returns current interrupt depth.

    Checks how deeply neested the caller is within interrupt handlers.

    \note
    See \ref irqs for more information on nested interrupts.

    \retval 0                   There is no interrupt active.
    \retval >0                  Caller is N interrupts deep.
    
    \sa irq_inside_int(), irq_active_int()
*/
size_t irq_int_depth(void);

/** Returns whether inside of an interrupt context.

    \retval true               If interrupt handling is in progress.
    \retval false              If normal processing is in progress.

    \sa irq_int_depth(), irq_active_int()
*/
bool irq_inside_int(void);

/** Returns the active IRQ source N levels deep.

    \note
    See \ref irqs for more information on nested interrupts.

    \param  level           0-based index for depth of the active IRQ
                            (0 is the current IRQ, depth-1 is first IRQ).

    \retval >0              Exception code for Nth IRQ context.
    \retval 0               No IRQ context is active.

    \sa irq_inside_int(), irq_int_depth()
*/
irq_t irq_active_int(size_t level);

/** Returns whether the current IRQ has been handled.

    Used to determine whether the active interrupt has been accepted by a
    handler callback or whether the next handler in the chain should be
    called.

    \note
    See \ref irqs for more information on nested interrupts.

    \param  level           0-based index for depth of the active IRQ
                            (0 is the current IRQ, depth-1 is first IRQ).

    \retval false           The active interrupt has not been handled OR
                            there is no currently active interrupt.
    \retval true            The active interrupt has been handled.

    \sa irq_handle_int()                            
*/
bool irq_handled_int(size_t level);

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

/** Get status register contents.

    Returns the current value of the status register, as irq_disable() does.
    The function can be found in arch\dreamcast\kernel\entry.s

    \note
    This is the entire status register word, not just the `IMASK` field.

    \retval                 Status register word

    \sa irq_disable()
*/
irq_mask_t irq_get_sr(void);

/** Disable interrupts.

    This function will disable SH4 interrupts, but will leave SH4 general
    exceptions enabled.

    \return                 The state of the SH4 interrupts before calling the
                            function. This can be used to restore this state
                            later on with irq_restore().

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

/** \brief  Disable interrupts with scope management.

    This macro will disable interrupts, similarly to irq_disable(), with the
    difference that the interrupt state will automatically be restored once the
    execution exits the functional block in which the macro was called.
*/
#define irq_disable_scoped() __irq_disable_scoped(__LINE__)

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

/** Accepts or declines to handle the active interrupt. 
  
    Used to signal back to top-level ISR dispatcher that the current IRQ has
    or hasn't been handled by the current handler.

    \note
    Regular IRQ handlers installed via irq_set_handler() automatically accept
    their interrupts, so this can be called with `false` to signal for them to
    be ignored.

    \note
    The special IRQ handlers for \ref EXC_UNHANDLED_EXC and
    \ref EXC_DOUBLE_FAULT are expected to ignore their exceptions by default,
    so this can be called with `true` for the interrupt to be accepted.

    \note
    The global handler does not accept its interrupts by default, so this
    can be called with `true` to signal for the interrupt to be accepted.

    \param handled          `true` to accept and handle the interrupt.
                            `false` to ignore and reject the interrupt.

    \sa irq_handled_int()
*/
void irq_handle_int(bool handled);

/** @} */

/** \defgroup irq_handlers  Handlers
    \brief                  API for managing IRQ handlers

    This API provides a series of methods for registering and retrieving
    different types of exception handlers.

    KOS's low-level IRQ dispatcher provides an extremely flexible interrupt
    handler chain with multiple ways to filter, accept, or reject a particular
    exception event.

    The interrupt handler chain provided by the dispatcher propagates
    unhandled exceptions to handlers in the following order:

        global handler -> specific handler -> unhandled exception handler

    Each step in the chain is free to either accept and terminate the
    exception, or ignore and reject it, causing the dispatcher to propagate
    it on to the next handler in the chain. Finally, the kernel will panic
    and abort if the exception made it through the chain without ever being
    handled.

    This exception propagation and handler dispatch mechanism provides for
    the following features:
        - <b>Conditional Exception Handling</b>: Any handler is free to
          determine whether to accept or ignore the exception at runtime.
        - <b>Exception Filtering</b>: Any handler is free to intercept an
          exception and mark it as handled before it propagates onwards.
        - <b>Exception Observing</b>: Any handler is free to simply "observe"
          the exception to log an event or do passive processing by not
          handling it and allowing it to propagate onwards.
        - <b>Multiple Exception Handling</b>: Using either global or unhandled
          exception handlers or by installing the same handler on multiple
          specific handlers, a single handler is free to implement logic
          for servicing multiple exceptions.

    \note
    The only exception to this rule is the \ref EXC_DOUBLE_FAULT software
    exception which is the only handler that fires in the case of an exception
    being raised while already in an interrupt handling an exception. If this
    handler does not handle the exception, the kernel panics and aborts. This
    is what is expected to happen by default, as KOS currently does not handle
    IRQs-within-IRQs getting separate contexts, so it's almost impossible to
    recover from such an event.

    @{
*/

/** The type of an IRQ handler.

    \param  code            The IRQ that caused the handler to be called.
    \param  context         The CPU's context.
    \param  data            Arbitrary userdata associated with the handler.
*/
typedef void (*irq_handler)(irq_t code, irq_context_t *context, void *data);


/** The type of a full callback of an IRQ handler and userdata.

    This type is used to set or get IRQ handlers and their data.
*/
typedef struct irq_cb {
    irq_handler hdl;    /**< A pointer to a procedure to handle an exception. */
    void       *data;   /**< A pointer that will be passed along to the callback. */
} irq_cb_t;

/** \defgroup irq_handlers_ind  Individual
    \brief                      API for managing individual IRQ handlers.

    This API is for managing handlers installed to handle individual IRQ codes.

    With the exception of \ref EXC_DOUBLE_FAULT, which is always called first,
    these individual exception handlers will be called \i after the global
    handler runs, assuming it does not accept the IRQ event. Finally, assuming
    an individual handler doesn't handle the event, the handler for
    \ref EXC_UNHANDLED_EXC will be called as a last effort to handle the
    exception.

    \note
    Any handler installed to handle any IRQ code type other than the
    `SOFT` type (\ref EXC_DOUBLE_FAULT and \ref EXC_UNHANDLED_EXC codes) is
    expected to have handled the given exception by default. If you wish to
    reject the exception or tell the dispatcher that it should remain
    unhandled, use irq_handle_int() with `FALSE`.

    \note
    The two `SOFT` type IRQs (\ref EXC_DOUBLE_FAULT and \ref EXC_UNHANDLED_EXC
    codes) are expected not to handle their exceptions by default, causing the
    system to panic and abort. If you wish to signal to the dispatcher that
    your handler has gracefully recovered from these exceptions, use
    irq_handle_int() with `TRUE` to signal that the exception was handled.

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

    \return                 The current handler for the IRQ type and
                            its userdata.

    \sa irq_set_handler()
*/
irq_cb_t irq_get_handler(irq_t code);

/** @} */

/** \defgroup irq_handlers_global   Global
    \brief                          API for managing global IRQ handler.

    @{
*/
/** Set a global exception handler.

    This function sets a global catch-all filter for all exception types.

    The global exception handler is called first for normal exception
    scenarios, giving it a chance to handle the exception request before it
    arrives at an individual-level handler. The exception to the rule is for
    when an exception is raised while already in an exception handler, in which
    case only the individual handler installed for \ref EXC_DOUBLE_FAULT will
    be called.

    \note
    By default the global exception handler is expected to reject or ignore the
    IRQ rather than handle it, causing it to continue propagating onwards to
    the individual/single interrupt handlers. If you wish to accept and handle
    the IRQ, terminating it, and preventing it from propagating onward, you
    must explicitly call irq_handle_int() with `TRUE`.


    \param  hnd             A pointer to the procedure to handle the exception.
    \param  data            A pointer that will be passed along to the callback.

    \retval 0               On success (no error conditions defined).

    \sa irq_get_global_filter(), irq_filter
*/
int irq_set_global_handler(irq_handler handler, void *data);

/** Get the global exception handler.

    \return                 The global exception handler and userdata set with
                            irq_set_global_handler(), or NULL if none is set.

    \sa irq_set_global_filter(), irq_filter
*/
irq_cb_t irq_get_global_handler(void);
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

/** \brief  Minimum/maximum values for IRQ priorities

    A priority of zero means the interrupt is masked.
    The maximum priority that can be set is 15.
 */
#define IRQ_PRIO_MAX    15
#define IRQ_PRIO_MIN    1
#define IRQ_PRIO_MASKED 0

/** \brief  Interrupt sources

   Interrupt sources at the SH4 level.
 */
typedef enum irq_src {
    IRQ_SRC_RTC,
    IRQ_SRC_TMU2,
    IRQ_SRC_TMU1,
    IRQ_SRC_TMU0,
    _IRQ_SRC_RESV,
    IRQ_SRC_SCI1,
    IRQ_SRC_REF,
    IRQ_SRC_WDT,
    IRQ_SRC_HUDI,
    IRQ_SRC_SCIF,
    IRQ_SRC_DMAC,
    IRQ_SRC_GPIO,
    IRQ_SRC_IRL3,
    IRQ_SRC_IRL2,
    IRQ_SRC_IRL1,
    IRQ_SRC_IRL0,
} irq_src_t;

/** \brief  Set the priority of a given IRQ source

    This function can be used to set the priority of a given IRQ source.

    \param  src             The interrupt source whose priority should be set
    \param  prio            The priority to set, in the range [0..15],
                            0 meaning the IRQs from that source are masked.
*/
void irq_set_priority(irq_src_t src, unsigned int prio);

/** \brief  Get the priority of a given IRQ source

    This function returns the priority of a given IRQ source.

    \param  src             The interrupt source whose priority should be set
    \return                 The priority of the IRQ source.
                            A value of 0 means the IRQs are masked.
*/
unsigned int irq_get_priority(irq_src_t src);

/** @} */

__END_DECLS

#endif  /* __ARCH_IRQ_H */
