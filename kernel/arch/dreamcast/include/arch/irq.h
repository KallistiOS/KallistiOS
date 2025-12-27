/* KallistiOS ##version##

   arch/dreamcast/include/arch/irq.h
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

    \see    dc/asic.h
*/

/* Keep this include above the macro guards */
#include <kos/irq.h>

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
struct __attribute__((aligned(32))) irq_context {
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
};

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
   |`TRAP ` | Virtual type (not a SH hardware type) for trap codes.

    List of exception codes:
*/
enum irq_exception
#ifdef __cplusplus
: unsigned int
#endif
{
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
    EXC_UNHANDLED_EXC      = 0x07e0, /**< `[SOFT  ]` Exception went unhandled */
    EXC_TRAP               = 0x0800  /**< `[TRAP  ]` Trap */
};

#define IRQ_TRAP_CODE(code) (EXC_TRAP + (code))

static inline int arch_irq_inside_int(void) {
    extern int inside_int;

    return inside_int;
}

static inline void arch_irq_restore(irq_mask_t old) {
    __asm__ volatile("ldc %0, sr" : : "r" (old));
}

static inline irq_mask_t arch_irq_disable(void) {
    irq_mask_t mask;

    __asm__ volatile("stc sr, %0" : "=r" (mask));

    arch_irq_restore((mask & 0xefffff0f) | 0x000000f0);
    return mask;
}

static inline void arch_irq_enable(void) {
    irq_mask_t mask;

    __asm__ volatile("stc sr, %0" : "=r" (mask));

    arch_irq_restore(mask & 0xefffff0f);
}

/** \defgroup irq_ctrl Control Flow
    \brief Methods for managing control flow within an irq_handler.

    This API provides methods for controlling program flow from within an
    active interrupt handler.

    @{
*/

void arch_irq_create_context(irq_context_t *context,
                             uintptr_t stack_pointer,
                             uintptr_t routine,
                             const uintptr_t *args);

int arch_irq_set_handler(irq_t code, irq_hdl_t hnd, void *data);

irq_cb_t arch_irq_get_handler(irq_t code);

int arch_irq_set_global_handler(irq_hdl_t hnd, void *data);

irq_cb_t arch_irq_get_global_handler(void);

void arch_irq_set_context(irq_context_t *cxt);

irq_context_t *arch_irq_get_context(void);

/** @} */

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
