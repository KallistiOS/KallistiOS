/* KallistiOS ##version##

   kernel/arch/dreamcast/include/dc/ubc.h
   Copyright (C) 2024 Falco Girgis
*/

/** \file    dc/ubc.h
    \brief   User Break Controller Driver 
    \ingroup ubc

    This file provides a driver and API around the SH4's UBC. 

    \sa arch/gdb.h

    \todo
    Add support for using the DBR register as the breakpoint handler.

    \author Falco Girgis
*/

#ifndef __DC_UBC_H
#define __DC_UBC_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdbool.h>
#include <stdint.h>

/** \defgroup ubc   User Break Controller
    \brief          Driver for the SH4's UBC
    \ingroup        debugging

    The SH4's User Break Controller (UBC) is a CPU peripheral which facilitates
    low-level software debugging. It provides two different channels which can
    be configured to monitor for certain memory or instruction conditions
    before generating a user-break interrupt. It provides the foundation for
    creating software-based debuggers and is the backing driver for the GDB
    debug stub.

    The following break comparison conditions are supported:
        - Address with optional ASID and 10, 12, 16, and 20-bit mask:
          supporting breaking on ranges of addresses and MMU operation.
        - Bus Cycle: supporting instruction or operand (data) breakpoints
        - Read/Write: supporting R, W, or RW access conditions.
        - Operand size: byte, word, longword, quadword
        - Data: 32-bit value with 32-bit mask for breaking on specific values
          or ranges of values (ubc_channel_b only).
        - Pre or Post-Instruction breaking

    \warning
    This Driver is used internally by the GDB stub, so care must be taken to
    not utilize the UBC during a GDB debugging session!

    @{
*/

/** \cond Forward declarations */
struct irq_context;
/** \endcond */

/** \brief UBC address mask specifier

    This value specifies which of the low bits are masked off and not included
    from ubc_breakpoint_t::address when configuring a breakpoint. By default,
    address masking is disabled, and the exact address given by
    ubc_breakpoint_t::address will be matched.

    \remark
    Using a mask allows you to break on a \a range of instructions or
    addresses.
*/
typedef enum ubc_address_mask {
    ubc_address_mask_none, /**< \brief Disable masking, all bits used */
    ubc_address_mask_10,   /**< \brief Mask off low 10 bits */
    ubc_address_mask_12,   /**< \brief Mask off low 12 bits */
    ubc_address_mask_16,   /**< \brief Mask off low 16 bits */
    ubc_address_mask_20,   /**< \brief Mask off low 20 bits */
    ubc_address_mask_all   /**< \brief Mask off all bits */
} ubc_address_mask_t;

/** \brief UBC access condition type specifier

    This value specifies whether to break when the address given by
    ubc_breakpoint_t::address is used as as an instruction, an operand, or
    either.

    \note
    Instruction access is an access that obtains an instruction while operand
    access is any memory access for the purpose of instruction execution. The
    default value is either access type.
*/
typedef enum ubc_access {
    ubc_access_either,      /**< \brief Instruction or operand */
    ubc_access_instruction, /**< \brief Instruction */
    ubc_access_operand      /**< \brief Operand */
} ubc_access_t;

/** \brief UBC read/write condition type specifier

    This value is used with operand-access breakpoints to further specify
    whether to break on read, write, or either access. The default value is
    either read or write.
*/
typedef enum ubc_rw {
    ubc_rw_either, /**< \brief Read or write */
    ubc_rw_read,   /**< \brief Read-only */
    ubc_rw_write   /**< \brief Write-only */
} ubc_rw_t;

/** \brief UBC size condition type specifier

    This value is used with operand-access breakpoints to further specify
    the size of the operand access to trigger the break condition. It defaults
    to breaking on any size.
*/
typedef enum ubc_size {
    ubc_size_any,   /**< \brief Any sizes */
    ubc_size_8bit,  /**< \brief Byte sizes */
    ubc_size_16bit, /**< \brief Word sizes */
    ubc_size_32bit, /**< \brief Longword sizes */
    ubc_size_64bit  /**< \brief Quadword sizes */
} ubc_size_t;

/** \brief UBC breakpoint structure

    This structure contains all of the information needed to configure a
    breakpoint using the SH4's UBC. It is meant to be zero-initialized,
    with the most commonly preferred, general values being the defaults,
    so that the only member that must be initialized to a non-zero value is
    ubc_breakpoint_t::address.

    \note
    The default configuration (from zero initialization) will trigger a
    breakpoint with any access to ubc_breakpoint_t::address.

    \warning
    When using ubc_breakpoint_t::asid or ubc_breakpoint_t::data, do not forget
    to set their respective `enable` members!
*/
typedef struct ubc_breakpoint {
    /** \brief Memory location of interest */
    void *address;

    /** \brief Conditional breakpoint settings */
    struct { 
        /** \brief Which bits of the address to break on */
        ubc_address_mask_t address_mask;

        /** \brief Which type of access to break on */
        ubc_access_t       access;

        /** \brief Read/write condition for operand-access breakpoints */
        ubc_rw_t           rw;

        /** \brief Size condition for operand-access breakpoints */
        ubc_size_t         size;
    } cond;

    /** \brief Optional ASID settings for when using the MMU */
    struct { 
        /** \brief Whether to enable ASID value comparisons */
        bool    enabled;

        /** \brief ASID value to match for the address */
        uint8_t value;
    } asid;

    /** \brief Optional operand data settings */
    struct {
        /** \brief Whether to enable data value comparisons */
        bool     enabled;

        /** \brief Comparison value for operand accesses */
        uint32_t value;

        /** \brief Which bits in the value are used in the data comparison */
        uint32_t mask;
    } data;

    /** \brief Optional instruction access type settings */
    struct {
        /** \brief Break after, not before, instruction access */
        bool break_after;
    } instr;

    /** \brief Next breakpoint in the sequence

        \warning
        You can only ever have a single sequential breakpoint active at a time,
        and the maximum sequence length is 2 chained breakpoints.
    */
    struct ubc_breakpoint *next;
} ubc_breakpoint_t;

/** \brief Breakpoint user callback

    Typedef for the user function to be invoked upon a encountering a
    breakpoint.

    \warning
    This callback is invoked within the context of an interrupt handler!

    \param  bp          Breakpoint that was encountered
    \param  ctx         Context of the current interrupt
    \param  user_data   User-supplied arbitrary callback data

    \retval true        Remove the breakpoint upon callback completion
    \retval false       Leave the breakpoint active upon callback completion
*/
typedef bool (*ubc_break_func_t)(const ubc_breakpoint_t   *bp, 
                                 const struct irq_context *ctx, 
                                 void                     *user_data);

/** \brief Enables a breakpoint

    Reserves a channel within the UBC for the given breakpoint.

    \param  bp          Configuration details for the breakpoint
    \param  callback    Handler which gets called upon breakpoint condition
    \param  user_data   Optional data to pass back to \p callback handler

    \retval true        The breakpoint was set successfully
    \retval false       The breakpoint failed to be set due to:
                            - Invalid configuration
                            - No available channel

    \sa ubc_remove_breakpoint()
*/
bool ubc_add_breakpoint(const ubc_breakpoint_t *bp,
                        ubc_break_func_t       callback,
                        void                   *user_data);

/** \brief Disables a breakpoint

    Removes a breakpoint from the UBC, freeing up a channel.

    \param  bp      The breakpoint to remove

    \retval true    The breakpoint was successfully removed
    \retval false   The breakpoint was not found

    \sa ubc_add_breakpoint(), ubc_clear_breakpoints()
*/
bool ubc_remove_breakpoint(const ubc_breakpoint_t *bp);

/** \brief Disables all active breakpoints

    Removes any breakpoints from the UBC, freeing up all channels.

    \note
    This is automatically called for you upon program termination.

    \sa ubc_remove_breakpoint()
*/
void ubc_clear_breakpoints(void);

/** \cond Called internally by KOS. */
void ubc_init(void);
void ubc_shutdown(void);
/** \endcond */

/** @} */

__END_DECLS

#endif  /* __DC_UBC_H */
