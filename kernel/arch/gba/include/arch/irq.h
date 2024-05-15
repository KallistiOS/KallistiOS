/* KallistiOS 2.0.0

   arch/gba/include/irq.h
   (c)2000-2001 Dan Potter

*/

#ifndef __ARCH_IRQ_H
#define __ARCH_IRQ_H

#include <arch/types.h>

/* The number of bytes required to save thread context. This should
   include all general CPU registers, FP registers, and status regs (even
   if not all of these are actually used). */
#define REG_BYTE_CNT 256            /* Currently really 228 */

/* Architecture-specific structure for holding the processor state (register
   values, etc). The size of this structure should be less than or equal
   to the above value. */
typedef struct irq_context {
    uint32  stuff;
} irq_context_t;

/* A couple of architecture independent access macros */
#define CONTEXT_PC(c)   ((c).stuff)
#define CONTEXT_FP(c)   ((c).stuff)
#define CONTEXT_SP(c)   ((c).stuff)
#define CONTEXT_RET(c)  ((c).stuff)

/* GBA-specific exception codes */

/* The value of the timer IRQ */
#define TIMER_IRQ       0

/* The type of an interrupt identifier */
typedef uint32 irq_t;

/* The type of an IRQ handler */
typedef void (*irq_handler)(irq_t source, irq_context_t *context);

/* Are we inside an interrupt handler? */
int irq_inside_int();

/* Pretend like we just came in from an interrupt and force
   a context switch back to the "current" context. Make sure
   you've called irq_set_context()! */
void irq_force_return();

/* Set a handler, or remove a handler (see codes above) */
int irq_set_handler(irq_t source, irq_handler hnd);

/* Set a global exception handler */
int irq_set_global_handler(irq_handler hnd);

/* Switch out contexts (for interrupt return) */
void irq_set_context(irq_context_t *regbank);

irq_handler irq_get_global_handler(void);
irq_handler irq_get_handler(irq_t source);

/* Return the current IRQ context */
irq_context_t *irq_get_context();

/* Fill a newly allocated context block for usage with supervisor/kernel
   or user mode. The given parameters will be passed to the called routine (up
   to the architecture maximum). */
void irq_create_context(irq_context_t *context, uint32 stack_pointer,
                        uint32 routine, uint32 *args, int usermode);

/* Enable/Disable interrupts */
int irq_disable();
void irq_enable();
void irq_restore(int v);

/* Init routine */
int irq_init();

/* Shutdown */
void irq_shutdown();

#endif  /* __ARCH_IRQ_H */

