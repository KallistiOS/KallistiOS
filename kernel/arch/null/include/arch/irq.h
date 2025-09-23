/* KallistiOS ##version##

   arch/null/include/arch/irq.h
   Copyright (C) 2000, 2001 Megan Potter
   Copyright (C) 2024 Paul Cercueil
   Copyright (C) 2024, 2025 Falco Girgis

*/

/** \file    arch/irq.h
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

#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <kos/cdefs.h>
__BEGIN_DECLS

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

/** \defgroup irq_context Context
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
    uint32_t  pc;         /**< Program counter */
    uint32_t  pr;         /**< Procedure register (aka return address) */
    uint32_t  sr;         /**< Status register */
    uint32_t  r[16];      /**< 16 general purpose (integer) registers */
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

/* Interrupt exception codes */
typedef unsigned int irq_t;

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

/** Get status register contents.

    Returns the current value of the status register, as irq_disable() does.
    The function can be found in arch\dreamcast\kernel\entry.s

    \note
    This is the entire status register word, not just the `IMASK` field.

    \retval                 Status register word
    \sa irq_disable()
*/
static inline irq_mask_t irq_get_sr(void) {
    //irq_mask_t value;
    //__asm__ volatile("stc sr, %0" : "=r" (value));
    return (irq_mask_t)0;
}

/** Restore IRQ state.

    This function will restore the interrupt state to the value specified. This
    should correspond to a value returned by irq_disable().

    \param  v               The IRQ state to restore. This should be a value
                            returned by irq_disable().

    \sa irq_disable()
*/
static inline void irq_restore(irq_mask_t old) {
    //__asm__ volatile("ldc %0, sr" : : "r" (old));
    (void)old;
}

/** Disable interrupts.

    This function will disable SH4 interrupts, but will leave SH4 general
    exceptions enabled.

    \return                 The state of the SH4 interrupts before calling the
                            function. This can be used to restore this state
                            later on with irq_restore().

    \sa irq_restore(), irq_enable()
*/
static inline irq_mask_t irq_disable(void) {
    uint32_t mask = (uint32_t)irq_get_sr();
    irq_restore((mask & 0xefffff0f) | 0x000000f0);
    return mask;
}

/** Enable all interrupts.

    This function will enable ALL interrupts, including external ones.

    \sa irq_disable()
*/
static inline void irq_enable(void) {
    uint32_t mask = ((uint32_t)irq_get_sr() & 0xefffff0f);
    irq_restore(mask);
}

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

    @{
*/

/** Set or remove an IRQ handler.

    Passing a NULL value for hnd will remove the current handler, if any.

    \param  code            The IRQ type to set the handler for
                            (see #irq_t).
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

    \note                   The specific handler will still be called for the
                            exception if one is set. If not, setting one of
                            these will stop the unhandled exception error.

    \param  hnd             A pointer to the procedure to handle the exception.
    \param  data            A pointer that will be passed along to the callback.

    \retval 0               On success (no error conditions defined).

*/
int irq_set_global_handler(irq_handler hnd, void *data);

/** Get the global exception handler.

    \return                 The global exception handler and userdata set with
                            irq_set_global_handler(), or NULL if none is set.
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

static inline void __irq_scoped_cleanup(irq_mask_t *state) {
    irq_restore(*state);
}

#define ___irq_disable_scoped(l) \
    irq_mask_t __scoped_irq_##l __attribute__((cleanup(__irq_scoped_cleanup))) = irq_disable()

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
typedef unsigned int irq_src_t;

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
