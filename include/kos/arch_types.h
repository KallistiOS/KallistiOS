/* KallistiOS ##version##

   include/kos/arch_types.h
   Copyright (C) 2025 Paul Cercueil
*/

/** \file    kos/arch_types.h
    \brief   Type definitions.
    \ingroup interrupts

    This file contains functions for enabling/disabling interrupts, and
    setting interrupt handlers.

    \author Paul Cercueil
*/
#ifndef __KOS_ARCH_TYPES_H
#define __KOS_ARCH_TYPES_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

enum irq_exception;
struct irq_context;

/** Architecture-specific structure for holding the processor state.

    This structure should hold register values and other important parts of the
    processor state.
*/
typedef struct irq_context irq_context_t;

/** Interrupt exception codes

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

__END_DECLS

#endif /* __KOS_ARCH_TYPES_H */
