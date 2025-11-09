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
#include <arch/irq.h>

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

    \param  v               The IRQ state to restore. This should be a value
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
