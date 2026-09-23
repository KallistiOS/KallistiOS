/* KallistiOS ##version##
   Copyright (C) 2026 Joseph Black
   Host declarations only; tests consume the real architecture context/helper.
*/
#ifndef TEST_IRQ_STACK_KOS_IRQ_H
#define TEST_IRQ_STACK_KOS_IRQ_H
#include <stdint.h>
typedef struct irq_context irq_context_t;
typedef uint32_t irq_t;
typedef uint32_t irq_mask_t;
typedef void (*irq_hdl_t)(irq_t, irq_context_t *, void *);
typedef struct irq_cb { irq_hdl_t hdl; void *data; } irq_cb_t;
#endif
