/* KallistiOS ##version##

   arch/dreamcast/include/arch/trap.h
   Copyright (C) 2024 Falco Girgis

*/

/** \file    arch/trap.h
    \brief   Interrupt and exception handling.
    \ingroup traps

    This file contains various definitions and declarations related to handling
    trap events, as are invoked through the `TRAPA` instruction.

    \author Falco Girgis

    \see    arch/irq.h, dc/asic.h

    \todo
        - state management, propagation
        - TRAPA instruction inline ASM
        - document reserved TRAP codes
*/

#ifndef __ARCH_TRAP_H
#define __ARCH_TRAP_H

#include <stdint.h>

#include <arch/irq.h>
#include <sys/cdefs.h>
__BEGIN_DECLS

/** \defgroup traps  Traps
    \brief    API for managing TRAPA events and handlers.
    \ingroup  system

    This API provides methods for setting and getting the installed handler for
    a particular `TRAPA` code.

    `TRAPA` is an SH4 instruction which simply takes a code (0-255) and fires
    an exception event which transfers execution to the kernel so that it can
    then be handled in software.

    @{
*/

/** Type for a code passed to the `TRAPA` instruction. */
typedef uint8_t trapa_t;

/** \defgroup irq_trapa_handler Handlers 
    \brief                      API for managing TRAPA handlers

    This API allows for the setting and retrieving of a handler associated with
    a particular `TRAPA` value. 

    @{
*/

/** The type of a TRAPA handler

    \param  code            The IRQ that caused the handler to be called.
    \param  context         The CPU's context.
    \param  data            Arbitrary userdata associated with the handler.
*/
typedef void (*trapa_handler)(trapa_t code, irq_context_t *context, void *data);

/** Set or remove a handler for a trapa code.
    
    \param  code            The value passed to the trapa opcode.
    \param  hnd             A pointer to the procedure to handle the trap.
    \param  data            A pointer that will be passed along to the callback.

    \retval 0               On success.

    \sa trapa_get_handler()
*/
int trapa_set_handler(trapa_t code, trapa_handler hnd, void *data);

/** Get an existing TRAPA handler.

    \param code             The value passed to the trapa opcode.
    \param data             A pointer to a void* which will be filled in with
                            the handler's userdata, or NULL if not interested.

    \return                 A pointer to the procedure to handle the TRAP code.

    \sa trapa_set_handler()
*/
trapa_handler trapa_get_handler(trapa_t code, void **data);

/** @} */

/** @} */

__END_DECLS

#endif /* __ARCH_TRAP_H */
