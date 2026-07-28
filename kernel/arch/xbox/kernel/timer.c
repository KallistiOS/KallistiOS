/* KallistiOS ##version##

   arch/xbox/kernel/timer.c
   Copyright (C) 2026 The KallistiOS Contributors
*/

#include <arch/arch.h>
#include <arch/timer.h>

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "pic.h"

/* Nominal Xbox clock expressed exactly as 2.2 GHz / 3. This form permits
   overflow-free conversion of the sub-second remainder to nanoseconds. */
#define XBOX_TSC_NUMERATOR_HZ 2200000000ULL
#define XBOX_TSC_DENOMINATOR  3ULL

#define XBOX_PIT_FREQUENCY_HZ 1193182ULL
#define XBOX_PIT_CHANNEL0     0x40U
#define XBOX_PIT_COMMAND      0x43U
#define XBOX_PIT_CHANNEL0_LOHI (3U << 4)
#define XBOX_PIT_MODE0         (0U << 1)
#define XBOX_PIT_MAX_TICKS     65536ULL

static uint64_t timer_tsc_base;
static uint64_t timer_primary_ticks_remaining;
static bool timer_initialized;
static timer_primary_callback_t timer_primary_callback;

static inline void timer_out8(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

static void timer_program_pit(uint8_t mode, uint16_t divisor) {
    timer_out8(XBOX_PIT_COMMAND, XBOX_PIT_CHANNEL0_LOHI | mode);
    timer_out8(XBOX_PIT_CHANNEL0, (uint8_t)divisor);
    timer_out8(XBOX_PIT_CHANNEL0, (uint8_t)(divisor >> 8));
}

static void timer_primary_irq(irq_t code, irq_context_t *context, void *data) {
    timer_primary_callback_t callback = timer_primary_callback;
    uint64_t chunk;

    (void)code;
    (void)data;

    if(timer_primary_ticks_remaining) {
        chunk = timer_primary_ticks_remaining;
        if(chunk > XBOX_PIT_MAX_TICKS)
            chunk = XBOX_PIT_MAX_TICKS;
        timer_primary_ticks_remaining -= chunk;
        timer_program_pit(XBOX_PIT_MODE0,
                          chunk == XBOX_PIT_MAX_TICKS
                              ? 0 : (uint16_t)chunk);
        return;
    }

    if(callback)
        callback(context);
}

/** Read the TSC after serializing prior instruction execution.

    CPUID is available on the Xbox CPU. It is used because this generation
    predates RDTSCP, and RDTSC alone is not a serializing instruction. */
static inline uint64_t timer_read_tsc(void) {
    uint32_t eax = 0;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t low;
    uint32_t high;

    __asm__ volatile("cpuid"
                     : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     :
                     : "memory");
    __asm__ volatile("rdtsc"
                     : "=a"(low), "=d"(high)
                     :
                     : "memory");

    return ((uint64_t)high << 32) | low;
}

int timer_init(void) {
    timer_primary_callback = NULL;
    timer_primary_ticks_remaining = 0;
    timer_tsc_base = timer_read_tsc();
    timer_initialized = true;
    return 0;
}

void timer_shutdown(void) {
    irq_mask_t old = arch_irq_disable();

    timer_primary_callback = NULL;
    timer_primary_ticks_remaining = 0;
    arch_irq_set_handler((irq_t)XBOX_PIC_MASTER_VECTOR, NULL, NULL);
    /* Leave channel 0 quiescent for the remainder of KOS shutdown. A count of
       zero represents 65536 ticks, giving shutdown code about 55 ms before a
       terminal edge even if interrupts are accidentally re-enabled. */
    timer_program_pit(XBOX_PIT_MODE0, 0);
    timer_initialized = false;
    arch_irq_restore(old);
}

struct timespec arch_timer_gettime(void) {
    uint64_t elapsed;
    uint64_t seconds;
    uint64_t remainder;
    uint64_t groups;
    uint64_t leftover;
    long nanoseconds;

    /* Permit early timing calls before arch_auto_init() exists. timer_init()
       will establish the official KOS epoch during normal initialization. */
    if(!timer_initialized)
        timer_init();

    elapsed = timer_read_tsc() - timer_tsc_base;
    seconds = elapsed / XBOX_TSC_HZ;
    remainder = elapsed % XBOX_TSC_HZ;

    /* At 2.2 GHz / 3, 2200 ticks represent exactly 3000 ns. Splitting the
       remainder first keeps every intermediate below 2^32. */
    groups = remainder / 2200ULL;
    leftover = remainder % 2200ULL;
    nanoseconds = (long)(groups * 3000ULL +
                         (leftover * 3000ULL) / 2200ULL);

    return (struct timespec) {
        .tv_sec = (time_t)seconds,
        .tv_nsec = nanoseconds,
    };
}

timer_primary_callback_t
timer_primary_set_callback(timer_primary_callback_t callback) {
    timer_primary_callback_t previous = timer_primary_callback;

    if(callback && !previous)
        arch_irq_set_handler((irq_t)XBOX_PIC_MASTER_VECTOR,
                             timer_primary_irq, NULL);
    else if(!callback && previous)
        arch_irq_set_handler((irq_t)XBOX_PIC_MASTER_VECTOR, NULL, NULL);
    timer_primary_callback = callback;
    return previous;
}

void timer_primary_wakeup(uint32_t millis) {
    uint64_t ticks;
    uint64_t chunk;
    irq_mask_t old;

    if(!timer_initialized || !timer_primary_callback)
        arch_panic("Xbox primary timer armed before initialization");

    if(!millis)
        millis = 1;
    ticks = (XBOX_PIT_FREQUENCY_HZ * millis + 999ULL) / 1000ULL;
    chunk = ticks;
    if(chunk > XBOX_PIT_MAX_TICKS)
        chunk = XBOX_PIT_MAX_TICKS;

    old = arch_irq_disable();
    timer_primary_ticks_remaining = ticks - chunk;
    timer_program_pit(XBOX_PIT_MODE0,
                      chunk == XBOX_PIT_MAX_TICKS ? 0 : (uint16_t)chunk);
    arch_irq_restore(old);
}
