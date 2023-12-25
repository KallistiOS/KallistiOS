/* KallistiOS ##version##

   kernel/arch/dreamcast/include/dc/ubc.h
   (C)2002 Megan Potter

*/

/** \file    dc/ubc.h
    \brief   User-break controller support.
    \ingroup ubc

    This file defines some functionality for using the SH4 UBC.

    \author Megan Potter
*/

#ifndef __DC_UBC_H
#define __DC_UBC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <arch/memory.h>

/** \defgroup ubc   User Break Controller
    \brief          Driver for the SH4's UBC
    \ingroup        debugging
*/

/* From the SH-4 PDF */
/** \defgroup   ubc_regs    Registers
    \brief                  User Break Controller Registers
    \ingroup                ubc

    These registers are as documented in the SH4 manual. Consult it for more
    information.

    @{
*/
#define BARA (*((vuint32*)SH4_REG_UBC_BARA))   /**< \brief BARA register. */
#define BASRA (*((vuint8*)SH4_REG_UBC_BASRA))  /**< \brief BASRA register. */
#define BAMRA (*((vuint8*)SH4_REG_UBC_BAMRA))  /**< \brief BAMRA register. */
#define BBRA (*((vuint16*)SH4_REG_UBC_BBRA))   /**< \brief BBRA register. */
#define BARB (*((vuint32*)SH4_REG_UBC_BARB))   /**< \brief BARB register. */
#define BASRB (*((vuint8*)SH4_REG_UBC_BDRB))   /**< \brief BASRB register. */
#define BAMRB (*((vuint8*)SH4_REG_UBC_BAMRB))  /**< \brief BAMRB register. */
#define BBRB (*((vuint16*)SH4_REG_UBC_BBRB))   /**< \brief BBRB register. */
#define BRCR (*((vuint16*)SH4_REG_UBC_BRCR))   /**< \brief BRCR register. */
/** @} */

/* These are inlined to avoid complications with using them */

/** \brief  Pause after setting UBC parameters. */
static inline void ubc_pause(void) {
    __asm__ __volatile__("nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n"
                         "nop\n");
}

/** \brief Disable all UBC breakpoints. */
static inline void ubc_disable_all(void) {
    BBRA = 0;
    BBRB = 0;
    ubc_pause();
}

/** \brief  Set a UBC data-write breakpoint at the given address.
    \param  address         The address to set the breakpoint at
*/
static inline void ubc_break_data_write(uint32 address) {
    BASRA = 0;      /* ASID = 0 */
    BARA = address;     /* Break address */
    BAMRA = 4;      /* Mask the ASID */
    BRCR = 0;       /* Nothing special, clear all flags */
    BBRA = 0x28;        /* Operand write cycle, no size constraint */
    ubc_pause();
}

/** \brief  Set a UBC instruction access breakpoint at the given address.
    \param  address         The address to set the breakpoint at.
    \param  use_dbr         Use the DBR register as the base for the exception.
*/
static inline void ubc_break_inst(uint32 address, int use_dbr) {
    BASRA = 0;      /* ASID = 0 */
    BARA = address; /* Break address */
    BAMRA = 4;      /* Mask the ASID */

    if(use_dbr) {
        BRCR = 1;   /* Use the DBR as the base for the IRQ */
    }
    else {
        BRCR = 0;
    }

    BBRA = 0x1C;    /* Instruction cycle, no size constraint */
    ubc_pause();
}

/* More to come.... */

__END_DECLS

#endif  /* __DC_UBC_H */

