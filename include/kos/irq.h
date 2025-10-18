/* KallistiOS ##version##

   include/kos/irq.h
   Copyright (C) 2000, 2001 Megan Potter
   Copyright (C) 2024, 2025 Paul Cercueil
   Copyright (C) 2024, 2025 Falco Girgis
*/

/** \file    kos/irq.h
    \brief   Timer functionality.
    \ingroup interrupts

    This file contains functions for enabling/disabling interrupts, and
    setting interrupt handlers.

    \author Megan Potter
    \author Paul Cercueil
    \author Falco Girgis
*/
#ifndef __KOS_IRQ_H
#define __KOS_IRQ_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdbool.h>
#include <stdint.h>

/** \cond INTERNAL */
struct irq_context;
#ifdef __cplusplus
enum irq_exception: unsigned int;
#else
enum irq_exception;
#endif
/** \endcond */

/** Architecture-specific structure for holding the processor state.

    This structure should hold register values and other important parts of the
    processor state.
*/
typedef struct irq_context irq_context_t;

/** Architecture-specific interrupt exception codes

   Used to identify the source or type of an interrupt.
*/
typedef enum irq_exception irq_t;

/** Type representing an interrupt mask state. */
typedef uint32_t irq_mask_t;

/** The type of an IRQ handler.

    \param  code            The IRQ that caused the handler to be called.
    \param  context         The CPU's context.
    \param  data            Arbitrary userdata associated with the handler.
*/
typedef void (*irq_hdl_t)(irq_t code, irq_context_t *context, void *data);


/** The type of a full callback of an IRQ handler and userdata.

    This type is used to set or get IRQ handlers and their data.
*/
typedef struct irq_cb {
    irq_hdl_t   hdl;    /**< A pointer to a procedure to handle an exception. */
    void       *data;   /**< A pointer that will be passed along to the callback. */
} irq_cb_t;

/* Keep this include after the type declarations */
#include <arch/irq.h>

/** \defgroup irq_state     IRQ state handling
    \brief                  API for handling IRQ state

    The following functions can be used to disable or enable interrupts, restore
    a previously saved interrupt state, or query whether or not the calling
    function is running in an interrupt context.

    @{
*/

/** Enable all interrupts.

    This function will enable ALL interrupts, including external ones.

    \sa irq_disable()
*/
static inline void irq_enable(void) {
    arch_irq_enable();
}

/** Disable interrupts.

    This function will disable interrupts (not exceptions).

    \return                 An opaque token containing the interrupt state.
                            It should be passed to irq_restore() in order to
                            restore the previous interrupt state.

    \sa irq_restore(), irq_enable()
*/
static inline irq_mask_t irq_disable(void) {
    return arch_irq_disable();
}

/** Restore interrupt state.

    This function will restore the interrupt state to the value specified. This
    should correspond to a value returned by irq_disable().

    \param  state           The IRQ state to restore. This should be a value
                            returned by irq_disable().

    \sa irq_disable()
*/
static inline void irq_restore(irq_mask_t state) {
    arch_irq_restore(state);
}

/** \brief  Disable interrupts with scope management.

    This macro will disable interrupts, similarly to irq_disable(), with the
    difference that the interrupt state will automatically be restored once the
    execution exits the functional block in which the macro was called.
*/
#define irq_disable_scoped() __irq_disable_scoped(__LINE__)

/** Returns whether inside of an interrupt context.

    \retval non-zero        If inside an interrupt handler.
    \retval 0               If normal processing is in progress.

*/
static inline bool irq_inside_int(void) {
    return arch_irq_inside_int();
}

/** @} */

/** \defgroup irq_context   IRQ context handling
    \brief                  API for handling IRQ contexts

    This API provides functions to create a new IRQ context, get a pointer to
    the current context, or set the context that will be used when returning
    from an exception.

    @{
*/

/** Fill a newly allocated context block.

    The given parameters will be passed to the called routine (up to the
    architecture maximum). For the Dreamcast, this maximum is 4.

    \param  context         The IRQ context to fill in.
    \param  stack_pointer   The value to set in the stack pointer.
    \param  routine         The address of the program counter for the context.
    \param  args            Any arguments to set in the registers. This cannot
                            be NULL, and must have enough values to fill in up
                            to the architecture maximum.
*/
static inline void irq_create_context(irq_context_t *context,
                                      uintptr_t stack_pointer,
                                      uintptr_t routine,
                                      const uintptr_t *args) {
    arch_irq_create_context(context, stack_pointer, routine, args);
}

/** Switch out contexts (for interrupt return).

    This function will set the processor state that will be restored when the
    exception returns.

    \param  cxt             The IRQ context to restore.

    \sa irq_get_context()
*/
static inline void irq_set_context(irq_context_t *cxt) {
    arch_irq_set_context(cxt);
}

/** Get the current IRQ context.

    This will fetch the processor context prior to the exception handling during
    an IRQ service routine.

    \return                 The current IRQ context.

    \sa irq_set_context()
*/
static inline irq_context_t *irq_get_context(void) {
    return arch_irq_get_context();
}

/** @} */

/** \defgroup irq_handlers  Handlers
    \brief                  API for managing IRQ handlers

    This API provides a series of methods for registering and retrieving
    different types of exception handlers.

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
static inline int irq_set_handler(irq_t code, irq_hdl_t hnd, void *data) {
    return arch_irq_set_handler(code, hnd, data);
}

/** Get the address of the current handler for the IRQ type.

    \param  code            The IRQ type to look up.

    \return                 The current handler for the IRQ type and
                            its userdata.

    \sa irq_set_handler()
*/
static inline irq_cb_t irq_get_handler(irq_t code) {
    return arch_irq_get_handler(code);
}

/** Set a global exception handler.

    This function sets a global catch-all filter for all exception types.

    \note                   The specific handler will still be called for the
                            exception if one is set. If not, setting one of
                            these will stop the unhandled exception error.

    \param  hnd             A pointer to the procedure to handle the exception.
    \param  data            A pointer that will be passed along to the callback.

    \retval 0               On success (no error conditions defined).

*/
static inline int irq_set_global_handler(irq_hdl_t hnd, void *data) {
    return arch_irq_set_global_handler(hnd, data);
}

/** Get the global exception handler.

    \return                 The global exception handler and userdata set with
                            irq_set_global_handler(), or NULL if none is set.
*/
static inline irq_cb_t irq_get_global_handler(void) {
    return arch_irq_get_global_handler();
}

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

__END_DECLS

#endif /* __KOS_IRQ_H */
