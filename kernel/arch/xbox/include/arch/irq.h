/* KallistiOS ##version##

   arch/xbox/include/arch/irq.h
   Copyright (C) 2000, 2001 Megan Potter
   Copyright (C) 2024 Paul Cercueil
   Copyright (C) 2024 Falco Girgis
   Copyright (C) 2026 Andress Barajas

*/

/** \file    arch/irq.h
    \brief   Interrupt and exception handling.
    \ingroup irqs

    This file contains various definitions and declarations related to handling
    interrupts and exceptions on the Xbox. This level deals with IRQs and
    exceptions generated on the x86 CPU (Pentium III), versus a higher layer
    which deals with differentiating "external" interrupts (APIC/PIC).

    \author Megan Potter
    \author Paul Cercueil
    \author Falco Girgis
    \author Andress Barajas
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
    \brief          IRQs and ISRs for the Xbox's x86 CPU
    \ingroup        system

    This is an API for managing interrupts, their masks, and their
    handler routines along with thread context information.

    \warning
    This is a low-level, internal kernel API. Care must be taken to not
    interfere with the IRQ handling which is being done by in-use KOS drivers.

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

    This should include all general CPU registers and status regs (even if not
    all of these are actually used).

    \note
    On the Xbox (x86-32) the register file below is small; we round the byte
    count up to a nicer number for sanity and future FPU/SSE state.
*/
#define REG_BYTE_CNT 256

/** Architecture-specific structure for holding the processor state.

    This models a standard x86-32 exception stack frame: the general-purpose
    register file, the stack/instruction pointers, the flags register, and the
    segment selectors, plus a slot for the exception vector and its (optional)
    hardware error code.

    \note
    The size of this structure should be less than or equal to the
    \ref REG_BYTE_CNT value.
*/
struct __attribute__((aligned(16))) irq_context {
    /* General-purpose registers (pusha order kept explicit for clarity). */
    uint32_t edi;         /**< Destination index register */
    uint32_t esi;         /**< Source index register */
    uint32_t ebp;         /**< Base (frame) pointer */
    uint32_t esp;         /**< Stack pointer */
    uint32_t ebx;         /**< General-purpose register B */
    uint32_t edx;         /**< General-purpose register D */
    uint32_t ecx;         /**< General-purpose register C */
    uint32_t eax;         /**< General-purpose register A / return value */

    /* Segment selectors. */
    uint32_t ds;          /**< Data segment selector */
    uint32_t es;          /**< Extra segment selector */
    uint32_t fs;          /**< FS segment selector (TLS base on x86) */
    uint32_t gs;          /**< GS segment selector */

    /* Exception identification. */
    uint32_t vector;      /**< Exception/interrupt vector number */
    uint32_t error_code;  /**< CPU-pushed error code (0 if none) */

    /* Interrupt stack frame pushed by the CPU. */
    uint32_t eip;         /**< Program counter (instruction pointer) */
    uint32_t cs;          /**< Code segment selector */
    uint32_t eflags;      /**< Flags register */
    uint32_t ss;          /**< Stack segment selector */
};

/** \name Register Accessors
    \brief Convenience macros for accessing context registers
    @{
*/
/** Fetch the program counter from an irq_context_t.
    \param  c               The context to read from.
    \return                 The program counter value.
*/
#define CONTEXT_PC(c)   ((c).eip)

/** Fetch the frame pointer from an irq_context_t.
    \param  c               The context to read from.
    \return                 The frame pointer value.
*/
#define CONTEXT_FP(c)   ((c).ebp)

/** Fetch the stack pointer from an irq_context_t.
    \param  c               The context to read from.
    \return                 The stack pointer value.
*/
#define CONTEXT_SP(c)   ((c).esp)

/** Fetch the return value from an irq_context_t.
    \param  c               The context to read from.
    \return                 The return value.
*/
#define CONTEXT_RET(c)  ((c).eax)
/** @} */

/** @} */

/** Interrupt/exception codes

    x86 protected-mode CPU exception vector numbers (0..31), plus a couple of
    KOS-internal software codes. Used to identify the source or type of an
    interrupt with irq_set_handler(), irq_get_handler(), etc.
*/
enum irq_exception
#ifdef __cplusplus
: unsigned int
#endif
{
    EXC_DIVIDE_ERROR       = 0x00, /**< Divide-by-zero error (#DE) */
    EXC_DEBUG              = 0x01, /**< Debug exception (#DB) */
    EXC_NMI                = 0x02, /**< Non-maskable interrupt */
    EXC_BREAKPOINT         = 0x03, /**< Breakpoint (#BP, INT3) */
    EXC_OVERFLOW           = 0x04, /**< Overflow (#OF, INTO) */
    EXC_BOUND_RANGE        = 0x05, /**< BOUND range exceeded (#BR) */
    EXC_INVALID_OPCODE     = 0x06, /**< Invalid opcode (#UD) */
    EXC_DEVICE_NOT_AVAIL   = 0x07, /**< Device not available (#NM) */
    EXC_DOUBLE_FAULT       = 0x08, /**< Double fault (#DF) */
    EXC_COPROC_SEG         = 0x09, /**< Coprocessor segment overrun */
    EXC_INVALID_TSS        = 0x0a, /**< Invalid TSS (#TS) */
    EXC_SEGMENT_NP         = 0x0b, /**< Segment not present (#NP) */
    EXC_STACK_FAULT        = 0x0c, /**< Stack-segment fault (#SS) */
    EXC_GP_FAULT           = 0x0d, /**< General protection fault (#GP) */
    EXC_PAGE_FAULT         = 0x0e, /**< Page fault (#PF) */
    EXC_X87_FPU            = 0x10, /**< x87 FPU floating-point error (#MF) */
    EXC_ALIGNMENT          = 0x11, /**< Alignment check (#AC) */
    EXC_MACHINE_CHECK      = 0x12, /**< Machine check (#MC) */
    EXC_SIMD_FPU           = 0x13, /**< SIMD floating-point exception (#XM) */

    EXC_UNHANDLED_EXC      = 0x00fe, /**< Exception went unhandled */
    EXC_TRAP               = 0x00ff  /**< Software trap base */
};

#define IRQ_TRAP_CODE(code) (irq_t)(EXC_TRAP + (code))

/** Whether we are currently inside an interrupt handler. */
extern int inside_int;

/** Returns whether inside of an interrupt context.

    \retval non-zero        If inside an interrupt handler.
    \retval 0               If normal processing is in progress.
*/
static inline int arch_irq_inside_int(void) {
    return inside_int;
}

/** \brief Restore IRQ state.

    Restores the CPU flags register (and thus the interrupt-enable IF bit) to
    the value previously returned by arch_irq_disable().

    \param  old             The IRQ state to restore.
*/
static inline void arch_irq_restore(irq_mask_t old) {
    __asm__ __volatile__("pushl %0\n\t"
                         "popfl"
                         : /* no output */
                         : "r"(old)
                         : "memory", "cc");
}

/** \brief Disable interrupts.

    Saves the current EFLAGS (so the IF state can be restored later) and then
    clears the interrupt-enable flag via `cli`.

    \return                 The prior EFLAGS value, to be passed to
                            arch_irq_restore().

    \sa arch_irq_restore(), arch_irq_enable()
*/
static inline irq_mask_t arch_irq_disable(void) {
    irq_mask_t old;

    __asm__ __volatile__("pushfl\n\t"
                         "popl %0\n\t"
                         "cli"
                         : "=r"(old)
                         : /* no input */
                         : "memory", "cc");
    return old;
}

/** \brief Enable all interrupts.

    Sets the x86 interrupt-enable flag via `sti`.

    \sa arch_irq_disable()
*/
static inline void arch_irq_enable(void) {
    __asm__ __volatile__("sti" : : : "memory", "cc");
}

/** \defgroup irq_ctrl Control Flow
    \brief Methods for managing control flow within an irq_handler.
    @{
*/

/** Fill a newly allocated context block.

    \param  context         The IRQ context to fill in.
    \param  stack_pointer   The value to set in the stack pointer.
    \param  routine         The address of the program counter for the context.
    \param  args            Any arguments to set in the registers.
*/
void arch_irq_create_context(irq_context_t *context,
                             uintptr_t stack_pointer,
                             uintptr_t routine,
                             const uintptr_t *args);

/** Set or remove an IRQ handler. */
int arch_irq_set_handler(irq_t code, irq_hdl_t hnd, void *data);

/** Get the address of the current handler for the IRQ type. */
irq_cb_t arch_irq_get_handler(irq_t code);

/** Set a global exception handler. */
int arch_irq_set_global_handler(irq_hdl_t hnd, void *data);

/** Get the global exception handler. */
irq_cb_t arch_irq_get_global_handler(void);

/** Switch out contexts (for interrupt return). */
void arch_irq_set_context(irq_context_t *cxt);

/** Get the current IRQ context. */
irq_context_t *arch_irq_get_context(void);

/** @} */

/** @} */

__END_DECLS

#endif  /* __ARCH_IRQ_H */
