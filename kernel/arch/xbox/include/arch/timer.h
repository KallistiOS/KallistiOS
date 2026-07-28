/* KallistiOS ##version##

   arch/xbox/include/arch/timer.h
   Copyright (C) 2026 The KallistiOS Contributors
*/

/** \file    arch/timer.h
    \brief   Original Xbox architecture timer interface.
    \ingroup timers
*/

#ifndef __ARCH_TIMER_H
#define __ARCH_TIMER_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <kos/irq.h>

#include <stdint.h>
#include <time.h>

/** Nominal retail Xbox timestamp-counter frequency.

    The CPU clock is nominally 2.2 GHz / 3. A physical retail Xbox probe
    measured the TSC at this rate within the uncertainty of network-based
    calibration. */
#define XBOX_TSC_HZ 733333333ULL

/** Return monotonic time elapsed since timer initialization. */
struct timespec arch_timer_gettime(void);

/** Callback invoked by the primary scheduler timer. */
typedef void (*timer_primary_callback_t)(irq_context_t *context);

/** Install the scheduler timer callback and return the previous callback. */
timer_primary_callback_t
timer_primary_set_callback(timer_primary_callback_t callback);

/** Request a scheduler callback after approximately \p millis milliseconds.

    PIT intervals longer than one 16-bit channel-0 countdown are delivered by
    chaining hardware one-shots before invoking the callback. */
void timer_primary_wakeup(uint32_t millis);

int timer_init(void);
void timer_shutdown(void);

__END_DECLS

#include <kos/timer.h>

#endif /* __ARCH_TIMER_H */
