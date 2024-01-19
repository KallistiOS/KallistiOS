/* KallistiOS ##version##

   kernel/arch/dreamcast/include/dc/ubc.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Falco Girgis
*/

/** \file    dc/ubc.h
    \brief   User Break Controller Driver 
    \ingroup ubc

    This file provides a driver and API around the SH4's UBC. 

    \sa arch/gdb.h

    \todo
    Add support for using the DBR register as the breakpoint handler.

    \author Megan Potter
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

#define UBC_BRK() __asm(    \
    /* "brk\n" */           \
    ".word 0x003B\n"        \
    "nop\n"                 \
) /* needs to be a constant, not known to the assembler */

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
    ubc_rw_either,  /**< \brief Read or write */
    ubc_rw_read,    /**< \brief Read-only */
    ubc_rw_write    /**< \brief Write-only */
} ubc_rw_t;

/** \brief UBC size condition type specifier

    This value is used with operand-access breakpoints to further specify
    the size of the operand access to trigger the break condition. It defaults
    to breaking on any size.
*/
typedef enum ubc_size {
    ubc_size_any,      /**< \brief Any sizes */
    ubc_size_byte,     /**< \brief 8-bit sizes */
    ubc_size_word,     /**< \brief 16-bit sizes */
    ubc_size_longword, /**< \brief 32-bit sizes */
    ubc_size_quadword  /**< \brief 64-bit sizes */
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
        /** \brief Which address bits to use */
        ubc_address_mask_t address_mask;

        /** \brief Which type of access to break on */
        ubc_access_t       access;

        /** \brief Read/write access condition */
        ubc_rw_t           rw;

        /** \brief Size access condition */
        ubc_size_t         size;
    } cond;

    /** \brief Optional ASID settings */
    struct { 
        /** \brief Whether to require the ASID value */
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

        /** \brief Mask for which bits in the value are used */
        uint32_t mask;
    } data;

    /** \brief Optional instruction access type settings */
    struct {
        /** \brief Break after, not before, instruction access */
        bool break_after;
    } instr;

    /** \brief Next breakpoint pointer, for sequential breakpoints

        \warning
        You can only ever have a single sequential breakpoint active at a time,
        and the maximum sequence length is 2 chained breakpoints.
    */
    struct ubc_breakpoint *next;
} ubc_breakpoint_t;

/** \brief UBC breakpoint user callback

    Typedef for the user function to be invoked upon a encountering a
    breakpoint.

    \warning
    This callback is invoked within the context of an interrupt handler!

    \param  bp          Breakpoint that was encountered
    \param  ctx         Context of the current interrupt
    \param  user_data   User-supplied arbitrary callback data

    \retval true        Unregister the handler upon callback completion
    \retval false       Leave the handler in-place upon callback completion
*/
typedef bool (*ubc_break_func_t)(const ubc_breakpoint_t   *bp, 
                                 const struct irq_context *ctx, 
                                 void                     *user_data);

bool ubc_enable_breakpoint(const ubc_breakpoint_t *bp,
                           ubc_break_func_t       callback,
                           void                   *user_data);

bool ubc_disable_breakpoint(const ubc_breakpoint_t *bp);


void ubc_set_break_handler(ubc_break_func_t callback,
                           void             *user_data);

void ubc_break(void);

/** \cond Called internally by KOS. */
void ubc_init(void);
void ubc_shutdown(void);
/** \endcond */

/** @} */

__END_DECLS

#endif  /* __DC_UBC_H */
