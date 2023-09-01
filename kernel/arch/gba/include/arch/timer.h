/* KallistiOS 2.0.0

   arch/gba/include/timer.h
   (c)2000-2001 Dan Potter

*/

#ifndef __ARCH_TIMER_H
#define __ARCH_TIMER_H

#include <arch/types.h>
#include <arch/irq.h>

/* Does the same as timer_ms_gettime(), but it merges both values
   into a single 64-bit millisecond counter. May be more handy
   in some situations. */
uint64 timer_ms_gettime64();

uint64 timer_us_gettime64();

void timer_ms_gettime(uint32* secs, uint32* msecs);

/* Set the callback target for the primary timer. Set to NULL
   to disable callbacks. Returns the address of the previous
   handler. */
typedef void (*timer_primary_callback_t)(irq_context_t *);
timer_primary_callback_t timer_primary_set_callback(timer_primary_callback_t callback);

/* Request a wakeup in approximately N milliseconds. You only get one
   simultaneous wakeup. Any subsequent calls here will replace any
   pending wakeup. */
void timer_primary_wakeup(uint32 millis);

/* Init function */
int timer_init();

/* Shutdown */
void timer_shutdown();

void timer_spin_sleep(int ms);

#endif  /* __ARCH_TIMER_H */

