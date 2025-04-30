/* KallistiOS ##version##

   arch/dreamcast/kernel/perfctr.c
   Copyright (C) 2023 Andy Barajas
   Copyright (C) 2023 Falco Girgis
*/

#include <dc/perfctr.h>
#include <arch/timer.h>

/* Control Registers */
#define PMCR_CTRL(o)  ( *((volatile uint16_t *)(0xff000084) + (o << 1)) )
#define PMCTR_HIGH(o) ( *((volatile uint32_t *)(0xff100004) + (o << 1)) )
#define PMCTR_LOW(o)  ( *((volatile uint32_t *)(0xff100008) + (o << 1)) )

/* PMCR Fields */
#define PMCR_PMENABLE   0x8000  /* Enable */
#define PMCR_PMST       0x4000  /* Start */
#define PMCR_RUN        0xc000  /* Run: Enable | Start */
#define PMCR_CLR        0x2000  /* Clear */
#define PMCR_PMCLK      0x0100  /* Clock Type */
#define PMCR_PMMODE     0x003f  /* Event Mode */

/* PMCR count type field position */
#define PMCR_PMCLK_SHIFT 8

/* 5ns per count in 1 cycle = 1 count mode(PMCR_COUNT_CPU_CYCLES) */
#define NS_PER_CYCLE      5

/* Get a counter's current configuration. */
bool perf_cntr_config(perf_cntr_t counter, 
                      perf_cntr_event_t *event,
                      perf_cntr_clock_t *clock) {

    const uint16_t config = PMCR_CTRL(counter);

    *event = (config & PMCR_PMMODE);
    *clock = (config & PMCR_PMCLK) >> PMCR_PMCLK_SHIFT;

    return (config & PMCR_RUN);
}

/* Start a performance counter. */
void perf_cntr_start(perf_cntr_t counter, 
                     perf_cntr_event_t event, 
                     perf_cntr_clock_t clock) {
    
    perf_cntr_clear(counter);

    PMCR_CTRL(counter) = PMCR_RUN | 
                        (clock << PMCR_PMCLK_SHIFT) | 
                        event;
}

/* Stop a performance counter. */
void perf_cntr_stop(perf_cntr_t counter) {
    PMCR_CTRL(counter) &= ~PMCR_RUN;
}

/* Resume a performance counter. */
void perf_cntr_resume(perf_cntr_t counter) {
    PMCR_CTRL(counter) |= PMCR_RUN;
}

/* Clears a performance counter.  Has to stop it first. */
void perf_cntr_clear(perf_cntr_t counter) {
    perf_cntr_stop(counter);
    PMCR_CTRL(counter) |= PMCR_CLR;
}

/* Returns the count value of a counter */
uint64_t perf_cntr_count(perf_cntr_t counter) {
    uint32_t lo, hi, hi2;

    do {
        /* Read the high part twice, before and after reading the high part,
         * to make sure that the low counter didn't overflow. We can detect an
         * overflow by the two reads of the high part returning different
         * values. */
        hi = PMCTR_HIGH(counter);
        lo = PMCTR_LOW(counter);
        hi2 = PMCTR_HIGH(counter);
    } while (__unlikely(hi != hi2));

    return (uint64_t)hi << 32 | lo;
}

void perf_cntr_timer_enable(void) {
    perf_cntr_start(PRFC0, PMCR_ELAPSED_TIME_MODE, PMCR_COUNT_CPU_CYCLES);
}

bool perf_cntr_timer_enabled(void) { 
    perf_cntr_event_t event;
    perf_cntr_clock_t clock;

    return (perf_cntr_config(PRFC0, &event, &clock) &&
            event == PMCR_ELAPSED_TIME_MODE &&
            clock == PMCR_COUNT_CPU_CYCLES);
}

void perf_cntr_timer_disable(void) {
    /* If timer is running, disable it */
    if(perf_cntr_timer_enabled()) {
        perf_cntr_clear(PRFC0);
    }
}

uint64_t perf_cntr_timer_ns(void) {
    /* If timer is configured and is running, use perf counters. */
    if(perf_cntr_timer_enabled())
        return perf_cntr_count(PRFC0) * NS_PER_CYCLE;
    else /* Otherwise fall-through to TMU2. */
        return timer_ns_gettime64();
}
