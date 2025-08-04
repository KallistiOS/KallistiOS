/*
   AICAOS

   irq.h
   Copyright (C) 2025 Paul Cercueil

   IRQ handling routines for the ARM
*/

#ifndef __AICAOS_IRQ_H
#define __AICAOS_IRQ_H

#include <registers.h>
#include <stdbool.h>

#define SPU_CPSR_F_BIT SPU_BIT(6)
#define SPU_CPSR_I_BIT SPU_BIT(7)

typedef unsigned int irq_ctx_t;

irq_ctx_t irq_disable(void);
void irq_restore(irq_ctx_t ctx);

static inline void __irq_scoped_cleanup(irq_ctx_t *state) {
    irq_restore(*state);
}

#define ___irq_disable_scoped(l) \
    irq_ctx_t __scoped_irq_##l __attribute__((cleanup(__irq_scoped_cleanup))) = irq_disable()

#define __irq_disable_scoped(l) ___irq_disable_scoped(l)

#define irq_disable_scoped() __irq_disable_scoped(__LINE__)

bool irq_enabled(void);

/* Interrupt the SH4 */
void aica_interrupt(void);

#endif /* __AICAOS_IRQ_H */
