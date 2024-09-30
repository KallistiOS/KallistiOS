/* KallistiOS ##version##

   arch/dreamcast/include/irq.h
   Copyright (C) 2000-2001 Megan Potter

*/

/** \file    arch/irq.h
    \brief   Interrupt and exception handling.
    \ingroup irqs

    This file contains various definitions and declarations related to handling
    interrupts and exceptions on the Dreamcast. This level deals with IRQs and
    exceptions generated on the SH4, versus the asic layer which deals with
    actually differentiating "external" interrupts.

    \author Megan Potter
    \see    dc/asic.h
*/

#ifndef __ARCH_IRQ_H
#define __ARCH_IRQ_H

#include <stdint.h>
#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/** \defgroup irqs  Interrupts
    \brief          IRQs and ISRs for the SH4's CPU
    \ingroup        system
*/

/** \brief   The number of bytes required to save thread context.
    \ingroup irqs

    This should include all general CPU registers, FP registers, and status regs
    (even if not all of these are actually used).

    On the Dreamcast, we need 228 bytes for all of that, but we round it up to a
    nicer number for sanity.
*/
#define REG_BYTE_CNT 256            /* Currently really 228 */

/** \brief   Architecture-specific structure for holding the processor state.
    \ingroup irqs

    This structure should hold register values and other important parts of the
    processor state. The size of this structure should be less than or equal
    to the REG_BYTE_CNT value.

    \headerfile arch/irq.h
*/
typedef struct irq_context {
    uint32_t  pc;         /**< \brief Program counter */
    uint32_t  pr;         /**< \brief Procedure register (aka return address) */
    uint32_t  gbr;        /**< \brief Global base register */
    uint32_t  vbr;        /**< \brief Vector base register */
    uint32_t  mach;       /**< \brief Multiply-and-accumulate register (high) */
    uint32_t  macl;       /**< \brief Multiply-and-accumulate register (low) */
    uint32_t  sr;         /**< \brief Status register */
    uint32_t  fpul;       /**< \brief Floatint-point communication register */
    uint32_t  fr[16];     /**< \brief Primary floating point registers */
    uint32_t  frbank[16]; /**< \brief Secondary floating point registers */
    uint32_t  r[16];      /**< \brief 16 general purpose (integer) registers */
    uint32_t  fpscr;      /**< \brief Floating-point status/control register */
} irq_context_t __attribute__((aligned(32)));

/* A couple of architecture independent access macros */
/** \brief   Fetch the program counter from an irq_context_t.
    \ingroup irqs

    \param  c               The context to read from.
    
    \return                 The program counter value.
*/
#define CONTEXT_PC(c)   ((c).pc)

/** \brief   Fetch the frame pointer from an irq_context_t.
    \ingroup irqs

    \param  c               The context to read from.
    
    \return                 The frame pointer value.
*/
#define CONTEXT_FP(c)   ((c).r[14])

/** \brief   Fetch the stack pointer from an irq_context_t.
    \ingroup irqs

    \param  c               The context to read from.
    
    \return                 The stack pointer value.
*/
#define CONTEXT_SP(c)   ((c).r[15])

/** \brief   Fetch the return value from an irq_context_t.
    \ingroup irqs
    
    \param  c               The context to read from.
    
    \return                 The return value.
*/
#define CONTEXT_RET(c)  ((c).r[0])

/** \defgroup irq_exception_codes   Exception Codes
    \brief                          IRQ exception code values
    \ingroup                        irqs

    These are all of the exceptions that can be raised on the SH4, and their
    codes. They're divided into several logical groups.

    @{
*/
/* Dreamcast-specific exception codes.. use these when getting or setting an
   exception code value. */

/** \defgroup irq_reset_codes       Reset Type
    \brief                          IRQ reset type codes

    These are exceptions that essentially cause a reset of the system. They
    cannot actually be caught by normal means. They will all automatically cause
    a branch to address 0xA0000000. These are pretty much fatal.

    @{
*/
#define EXC_RESET_POWERON   0x0000  /**< \brief Power-on reset */
#define EXC_RESET_MANUAL    0x0020  /**< \brief Manual reset */
#define EXC_RESET_UDI       0x0000  /**< \brief Hitachi UDI reset */
#define EXC_ITLB_MULTIPLE   0x0140  /**< \brief Instruction TLB multiple hit */
#define EXC_DTLB_MULTIPLE   0x0140  /**< \brief Data TLB multiple hit */
/** @} */

/** \defgroup irq_reexec_codes      Re-Execution Type
    \brief                          IRQ re-execution type codes

    These exceptions will stop the currently processing instruction, and
    transition into exception processing. After handling the exception (assuming
    that it can be handled by the code), the offending instruction will be
    re-executed from the start.

    @{
*/
#define EXC_USER_BREAK_PRE      0x01e0  /**< \brief User break before instruction */
#define EXC_INSTR_ADDRESS       0x00e0  /**< \brief Instruction address */
#define EXC_ITLB_MISS           0x0040  /**< \brief Instruction TLB miss */
#define EXC_ITLB_PV             0x00a0  /**< \brief Instruction TLB protection violation */
#define EXC_ILLEGAL_INSTR       0x0180  /**< \brief Illegal instruction */
#define EXC_SLOT_ILLEGAL_INSTR  0x01a0  /**< \brief Slot illegal instruction */
#define EXC_GENERAL_FPU         0x0800  /**< \brief General FPU exception */
#define EXC_SLOT_FPU            0x0820  /**< \brief Slot FPU exception */
#define EXC_DATA_ADDRESS_READ   0x00e0  /**< \brief Data address (read) */
#define EXC_DATA_ADDRESS_WRITE  0x0100  /**< \brief Data address (write) */
#define EXC_DTLB_MISS_READ      0x0040  /**< \brief Data TLB miss (read) */
#define EXC_DTLB_MISS_WRITE     0x0060  /**< \brief Data TLB miss (write) */
#define EXC_DTLB_PV_READ        0x00a0  /**< \brief Data TLB protection violation (read) */
#define EXC_DTLB_PV_WRITE       0x00c0  /**< \brief Data TLB protection violation (write) */
#define EXC_FPU                 0x0120  /**< \brief FPU exception */
#define EXC_INITIAL_PAGE_WRITE  0x0080  /**< \brief Initial page write exception */
/** @} */

/** \defgroup irq_completion_codes  Completion Type
    \brief                          IRQ completion type codes

    These exceptions are actually handled in-between instructions, allowing the
    instruction that causes them to finish completely. The saved PC thus is the
    value of the next instruction.

    @{
*/
#define EXC_TRAPA           0x0160  /**< \brief Unconditional trap (trapa) */
#define EXC_USER_BREAK_POST 0x01e0  /**< \brief User break after instruction */
/** @} */

/** \defgroup irq_interrupt_codes   Interrupt (Completion Type)
    \brief                          IRQ interrupt completiion type codes

    \note   Not all of these have any meaning on the Dreamcast. Those that have
            no meaning are only included for completeness.

    These exceptions are caused by interrupt requests. These generally are from
    peripheral devices, but NMIs, timer interrupts, and DMAC interrupts are also
    included here.

    @{
*/
#define EXC_NMI         0x01c0  /**< \brief Nonmaskable interrupt */
#define EXC_IRQ0        0x0200  /**< \brief External IRQ request (level 0) */
#define EXC_IRQ1        0x0220  /**< \brief External IRQ request (level 1) */
#define EXC_IRQ2        0x0240  /**< \brief External IRQ request (level 2) */
#define EXC_IRQ3        0x0260  /**< \brief External IRQ request (level 3) */
#define EXC_IRQ4        0x0280  /**< \brief External IRQ request (level 4) */
#define EXC_IRQ5        0x02a0  /**< \brief External IRQ request (level 5) */
#define EXC_IRQ6        0x02c0  /**< \brief External IRQ request (level 6) */
#define EXC_IRQ7        0x02e0  /**< \brief External IRQ request (level 7) */
#define EXC_IRQ8        0x0300  /**< \brief External IRQ request (level 8) */
#define EXC_IRQ9        0x0320  /**< \brief External IRQ request (level 9) */
#define EXC_IRQA        0x0340  /**< \brief External IRQ request (level 10) */
#define EXC_IRQB        0x0360  /**< \brief External IRQ request (level 11) */
#define EXC_IRQC        0x0380  /**< \brief External IRQ request (level 12) */
#define EXC_IRQD        0x03a0  /**< \brief External IRQ request (level 13) */
#define EXC_IRQE        0x03c0  /**< \brief External IRQ request (level 14) */
#define EXC_TMU0_TUNI0  0x0400  /**< \brief TMU0 underflow */
#define EXC_TMU1_TUNI1  0x0420  /**< \brief TMU1 underflow */
#define EXC_TMU2_TUNI2  0x0440  /**< \brief TMU2 underflow */
#define EXC_TMU2_TICPI2 0x0460  /**< \brief TMU2 input capture */
#define EXC_RTC_ATI     0x0480  /**< \brief RTC alarm interrupt */
#define EXC_RTC_PRI     0x04a0  /**< \brief RTC periodic interrupt */
#define EXC_RTC_CUI     0x04c0  /**< \brief RTC carry interrupt */
#define EXC_SCI_ERI     0x04e0  /**< \brief SCI Error receive */
#define EXC_SCI_RXI     0x0500  /**< \brief SCI Receive ready */
#define EXC_SCI_TXI     0x0520  /**< \brief SCI Transmit ready */
#define EXC_SCI_TEI     0x0540  /**< \brief SCI Transmit error */
#define EXC_WDT_ITI     0x0560  /**< \brief Watchdog timer */
#define EXC_REF_RCMI    0x0580  /**< \brief Memory refresh compare-match interrupt */
#define EXC_REF_ROVI    0x05a0  /**< \brief Memory refresh counter overflow interrupt */
#define EXC_UDI         0x0600  /**< \brief Hitachi UDI */
#define EXC_GPIO_GPIOI  0x0620  /**< \brief I/O port interrupt */
#define EXC_DMAC_DMTE0  0x0640  /**< \brief DMAC transfer end (channel 0) */
#define EXC_DMAC_DMTE1  0x0660  /**< \brief DMAC transfer end (channel 1) */
#define EXC_DMAC_DMTE2  0x0680  /**< \brief DMAC transfer end (channel 2) */
#define EXC_DMAC_DMTE3  0x06a0  /**< \brief DMAC transfer end (channel 3) */
#define EXC_DMA_DMAE    0x06c0  /**< \brief DMAC address error */
#define EXC_SCIF_ERI    0x0700  /**< \brief SCIF Error receive */
#define EXC_SCIF_RXI    0x0720  /**< \brief SCIF Receive ready */
#define EXC_SCIF_BRI    0x0740  /**< \brief SCIF break */
#define EXC_SCIF_TXI    0x0760  /**< \brief SCIF Transmit ready */
/** @} */

/** \brief  Double fault

    This exception is completely done in software (not represented on the CPU at
    all). Its used for when an exception occurs during an IRQ service routine.
*/
#define EXC_DOUBLE_FAULT    0x0ff0

/** \brief  Unhandled exception

    This exception is a software-generated exception for a generic unhandled
    exception.
*/
#define EXC_UNHANDLED_EXC   0x0fe0
/** @} */

/** \defgroup  irq_type_offsets        Exception type offsets
    \brief                             Offsets within exception types
    \ingroup                           irqs

    The following are a table of "type offsets" (see the Hitachi PDF). These are
    the 0x000, 0x100, 0x400, and 0x600 offsets.

    @{
*/
#define EXC_OFFSET_000  0   /**< \brief Offset 0x000 */
#define EXC_OFFSET_100  1   /**< \brief Offset 0x100 */
#define EXC_OFFSET_400  2   /**< \brief Offset 0x400 */
#define EXC_OFFSET_600  3   /**< \brief Offset 0x600 */
/** @} */

/** \brief   The value of the timer IRQ
    \ingroup irqs
*/
#define TIMER_IRQ       EXC_TMU0_TUNI0

/** \brief   The type of an interrupt identifier 
    \ingroup irqs
*/
typedef uint32_t irq_t;

/** \brief   The type of an IRQ handler
    \ingroup irqs 

    \param  source          The IRQ that caused the handler to be called.
    \param  context         The CPU's context.
*/
typedef void (*irq_handler)(irq_t source, irq_context_t *context, void *data);

/** \brief   Are we inside an interrupt handler?
    \ingroup irqs

    \retval 1               If interrupt handling is in progress.
    \retval 0               If normal processing is in progress.
*/
int irq_inside_int(void);

/** \brief   Pretend like we just came in from an interrupt and force
             a context switch back to the "current" context.
    \ingroup irqs

    \warning
    Make sure you've called irq_set_context() before doing this!
*/
void irq_force_return(void);

/** \brief   Set or remove an IRQ handler.
    \ingroup irqs
    
    Passing a NULL value for hnd will remove the current handler, if any.

    \param  source          The IRQ type to set the handler for
                            (see \ref irq_exception_codes).
    \param  hnd             A pointer to a procedure to handle the exception.
    \param  data            A pointer that will be passed along to the callback.
    
    \retval 0               On success.
    \retval -1              If the source is invalid.
*/
int irq_set_handler(irq_t source, irq_handler hnd, void *data);

/** \brief   Get the address of the current handler for the IRQ type.
    \ingroup irqs

    \param  source          The IRQ type to look up.
    
    \return                 A pointer to the procedure to handle the exception.
*/
irq_handler irq_get_handler(irq_t source);

/** \brief   Set or remove a handler for a trapa code.
    \ingroup irqs
    
    \param  code            The value passed to the trapa opcode.
    \param  hnd             A pointer to the procedure to handle the trap.
    \param  data            A pointer that will be passed along to the callback.

    \retval 0               On success.
    \retval -1              If the code is invalid (greater than 0xFF).
*/
int trapa_set_handler(irq_t code, irq_handler hnd, void *data);

/** \brief   Set a global exception handler.
    \ingroup irqs

    This function sets a global catch-all handler for all exception types.

    \note                   The specific handler will still be called for the
                            exception if one is set. If not, setting one of
                            these will stop the unhandled exception error.

    \param  hnd             A pointer to the procedure to handle the exception.
    \param  data            A pointer that will be passed along to the callback.

    \retval 0               On success (no error conditions defined).
*/
int irq_set_global_handler(irq_handler hnd, void *data);

/** \brief   Get the global exception handler.
    \ingroup irqs

    \return                 The global exception handler set with
                            irq_set_global_handler(), or NULL if none is set.
*/
irq_handler irq_get_global_handler(void);

/** \brief   Switch out contexts (for interrupt return).
    \ingroup irqs

    This function will set the processor state that will be restored when the
    exception returns.

    \param  regbank         The values of all registers to be restored.
*/
void irq_set_context(irq_context_t *regbank);

/** \brief   Get the current IRQ context.
    \ingroup irqs

    This will fetch the processor context prior to the exception handling during
    an IRQ service routine.

    \return                 The current IRQ context.
*/
irq_context_t *irq_get_context(void);

/** \brief   Fill a newly allocated context block for usage with supervisor
             or user mode.
    \ingroup irqs

    The given parameters will be passed to the called routine (up to the
    architecture maximum). For the Dreamcast, this maximum is 4.

    \param  context         The IRQ context to fill in.
    \param  stack_pointer   The value to set in the stack pointer.
    \param  routine         The address of the program counter for the context.
    \param  args            Any arguments to set in the registers. This cannot
                            be NULL, and must have enough values to fill in up
                            to the architecture maximum.
    \param  usermode        1 to run the routine in user mode, 0 for supervisor.
*/
void irq_create_context(irq_context_t *context, uint32_t stack_pointer,
                        uint32_t routine, uint32_t *args, int usermode);

/* Enable/Disable interrupts */
/** \brief   Disable interrupts.
    \ingroup irqs

    This function will disable interrupts, but will leave exceptions enabled.

    \return                 The state of IRQs before calling the function. This
                            can be used to restore this state later on with
                            irq_restore().
*/
int irq_disable(void);

/** \brief   Enable all interrupts.
    \ingroup irqs

    This function will enable ALL interrupts, including external ones.
*/
void irq_enable(void);

/** \brief   Restore IRQ state.
    \ingroup irqs

    This function will restore the interrupt state to the value specified. This
    should correspond to a value returned by irq_disable().

    \param  v               The IRQ state to restore. This should be a value
                            returned by irq_disable().
*/
void irq_restore(int v);

/** \brief   Initialize interrupts.
    \ingroup irqs

    \retval 0               On success (no error conditions defined).
*/
int irq_init(void);

/** \brief   Shutdown interrupts, restoring the state to how it was before
             irq_init() was called.
    \ingroup irqs
*/
void irq_shutdown(void);

/** \cond */
static inline void __irq_scoped_cleanup(int *state) {
    irq_restore(*state);
}

#define ___irq_disable_scoped(l) \
    int __scoped_irq_##l __attribute__((cleanup(__irq_scoped_cleanup))) = irq_disable()

#define __irq_disable_scoped(l) ___irq_disable_scoped(l)
/** \endcond */

/** \brief  Disable interrupts with scope management.

    This macro will disable interrupts, similarly to irq_disable(), with the
    difference that the interrupt state will automatically be restored once the
    execution exits the functional block in which the macro was called.
*/
#define irq_disable_scoped() __irq_disable_scoped(__LINE__)

__END_DECLS

#endif  /* __ARCH_IRQ_H */
