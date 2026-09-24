#ifndef __KOS_IRQ_H
#define __KOS_IRQ_H

#include <stdbool.h>

extern bool test_in_irq;
static inline bool irq_inside_int(void) {
    return test_in_irq;
}

#endif
