/* KallistiOS ##version##

   timer.c
   Copyright (c)2000, 2001, 2002 Megan Potter
*/

#include <assert.h>

#include <stdio.h>
#include <arch/arch.h>
#include <arch/timer.h>
#include <arch/irq.h>

/* Quick access macros */
#define TIMER8(o) ( *((volatile uint8*)(0xffd80000 + (o))) )
#define TIMER16(o) ( *((volatile uint16*)(0xffd80000 + (o))) )
#define TIMER32(o) ( *((volatile uint32*)(0xffd80000 + (o))) )
#define TOCR    0x00
#define TSTR    0x04
#define TCOR0   0x08
#define TCNT0   0x0c
#define TCR0    0x10
#define TCOR1   0x14
#define TCNT1   0x18
#define TCR1    0x1c
#define TCOR2   0x20
#define TCNT2   0x24
#define TCR2    0x28
#define TCPR2   0x2c

static int tcors[] = { TCOR0, TCOR1, TCOR2 };
static int tcnts[] = { TCNT0, TCNT1, TCNT2 };
static int tcrs[] = { TCR0, TCR1, TCR2 };

/* Pre-initialize a timer; set values but don't start it */
int timer_prime(int which, uint32 speed, int interrupts) {
    /* P0/64 scalar, maybe interrupts */
    if(interrupts)
        TIMER16(tcrs[which]) = 32 | 2;
    else
        TIMER16(tcrs[which]) = 2;

    /* Initialize counters; formula is P0/(tps*64) */
    TIMER32(tcnts[which]) = 50000000 / (speed * 64);
    TIMER32(tcors[which]) = 50000000 / (speed * 64);

    if(interrupts)
        timer_enable_ints(which);

    return 0;
}

/* Works like timer_prime, but takes an interval in milliseconds
   instead of a rate. Used by the primary timer stuff. */
static int timer_prime_wait(int which, uint32 millis, int interrupts) {
    /* Calculate the countdown, formula is P0 * millis/64000. We
       rearrange the math a bit here to avoid integer overflows. */
    uint32 cd = (50000000 / 64) * millis / 1000;

    /* P0/64 scalar, maybe interrupts */
    if(interrupts)
        TIMER16(tcrs[which]) = 32 | 2;
    else
        TIMER16(tcrs[which]) = 2;

    /* Initialize counters */
    TIMER32(tcnts[which]) = cd;
    TIMER32(tcors[which]) = cd;

    if(interrupts)
        timer_enable_ints(which);

    return 0;
}

/* Start a timer -- starts it running (and interrupts if applicable) */
int timer_start(int which) {
    TIMER8(TSTR) |= 1 << which;
    return 0;
}

/* Stop a timer -- and disables its interrupt */
int timer_stop(int which) {
    timer_disable_ints(which);

    /* Stop timer */
    TIMER8(TSTR) &= ~(1 << which);

    return 0;
}

/* Returns the count value of a timer */
uint32 timer_count(int which) {
    return TIMER32(tcnts[which]);
}

/* Clears the timer underflow bit and returns what its value was */
int timer_clear(int which) {
    uint16 value = TIMER16(tcrs[which]);
    TIMER16(tcrs[which]) &= ~0x100;

    return (value & 0x100) ? 1 : 0;
}

/* Spin-loop kernel sleep func: uses the secondary timer in the
   SH-4 to very accurately delay even when interrupts are disabled */
void timer_spin_sleep(int ms) {
    timer_prime(TMU1, 1000, 0);
    timer_clear(TMU1);
    timer_start(TMU1);

    while(ms > 0) {
        while(!(TIMER16(tcrs[TMU1]) & 0x100))
            ;

        timer_clear(TMU1);
        ms--;
    }

    timer_stop(TMU1);
}

/* Enable timer interrupts (high priority); needs to move
   to irq.c sometime. */
void timer_enable_ints(int which) {
    volatile uint16 *ipra = (uint16*)0xffd00004;
    *ipra |= (0x000f << (12 - 4 * which));
}

/* Disable timer interrupts; needs to move to irq.c sometime. */
void timer_disable_ints(int which) {
    volatile uint16 *ipra = (uint16*)0xffd00004;
    *ipra &= ~(0x000f << (12 - 4 * which));
}

/* Check whether ints are enabled */
int timer_ints_enabled(int which) {
    volatile uint16 *ipra = (uint16*)0xffd00004;
    return (*ipra & (0x000f << (12 - 4 * which))) != 0;
}

/* Millisecond timer */
static uint32 timer_ms_counter = 0;
static uint32 timer_ms_countdown;
static void timer_ms_handler(irq_t source, irq_context_t *context) {
    (void)source;
    (void)context;
    timer_ms_counter++;
}

void timer_ms_enable() {
    irq_set_handler(EXC_TMU2_TUNI2, timer_ms_handler);
    timer_prime(TMU2, 1, 1);
    timer_ms_countdown = timer_count(TMU2);
    timer_clear(TMU2);
    timer_start(TMU2);
}

void timer_ms_disable() {
    timer_stop(TMU2);
    timer_disable_ints(TMU2);
}

/* Return the number of ticks since KOS was booted */
void timer_ms_gettime(uint32 *secs, uint32 *msecs) {
    uint32 used;

    /* Seconds part comes from ms_counter */
    if(secs)
        *secs = timer_ms_counter;

    /* Milliseconds, we check how much of the timer has elapsed */
    if(msecs) {
        assert(timer_ms_countdown > 0);
        used = timer_ms_countdown - timer_count(TMU2);
        *msecs = used * 1000 / timer_ms_countdown;
    }
}

uint64 timer_ms_gettime64() {
    uint32 s, ms;
    uint64 msec;

    timer_ms_gettime(&s, &ms);
    msec = ((uint64)s) * 1000 + ((uint64)ms);

    return msec;
}

uint64 timer_us_gettime64() {
    uint32 cnt, scnt;
    uint64 usec;
    uint64 used;

    scnt = timer_ms_counter;
    cnt = timer_count(TMU2);

    assert(timer_ms_countdown > 0);
    used = timer_ms_countdown - cnt;
    usec = scnt * 1000000;
    usec += used * 1000000 / timer_ms_countdown;

    return usec;
}

/* Primary kernel timer. What we'll do here is handle actual timer IRQs
   internally, and call the callback only after the appropriate number of
   millis has passed. For the DC you can't have timers spaced out more
   than about one second, so we emulate longer waits with a counter. */
static timer_primary_callback_t tp_callback;
static uint32 tp_ms_remaining;

/* IRQ handler for the primary timer interrupt. */
static void tp_handler(irq_t src, irq_context_t * cxt) {
    (void)src;

    /* Are we at zero? */
    if(tp_ms_remaining == 0) {
        /* Disable any further timer events. The callback may
           re-enable them of course. */
        timer_stop(TMU0);
        timer_disable_ints(TMU0);

        /* Call the callback, if any */
        if(tp_callback)
            tp_callback(cxt);
    } /* Do we have less than a second remaining? */
    else if(tp_ms_remaining < 1000) {
        /* Schedule a "last leg" timer. */
        timer_stop(TMU0);
        timer_prime_wait(TMU0, tp_ms_remaining, 1);
        timer_clear(TMU0);
        timer_start(TMU0);
        tp_ms_remaining = 0;
    } /* Otherwise, we're just counting down. */
    else {
        tp_ms_remaining -= 1000;
    }
}

/* Enable / Disable primary kernel timer */
static void timer_primary_init() {
    /* Clear out our vars */
    tp_callback = NULL;

    /* Clear out TMU0 and get ready for wakeups */
    irq_set_handler(EXC_TMU0_TUNI0, tp_handler);
    timer_clear(TMU0);
}

static void timer_primary_shutdown() {
    timer_stop(TMU0);
    timer_disable_ints(TMU0);
    irq_set_handler(EXC_TMU0_TUNI0, NULL);
}

timer_primary_callback_t timer_primary_set_callback(timer_primary_callback_t cb) {
    timer_primary_callback_t cbold = tp_callback;
    tp_callback = cb;
    return cbold;
}

void timer_primary_wakeup(uint32 millis) {
    /* Don't allow zero */
    if(millis == 0) {
        assert_msg(millis != 0, "Received invalid wakeup delay");
        millis++;
    }

    /* Make sure we stop any previous wakeup */
    timer_stop(TMU0);

    /* If we have less than a second to wait, then just schedule the
       timeout event directly. Otherwise schedule a periodic second
       timer. We'll replace this on the last leg in the IRQ. */
    if(millis >= 1000) {
        timer_prime_wait(TMU0, 1000, 1);
        timer_clear(TMU0);
        timer_start(TMU0);
        tp_ms_remaining = millis - 1000;
    }
    else {
        timer_prime_wait(TMU0, millis, 1);
        timer_clear(TMU0);
        timer_start(TMU0);
        tp_ms_remaining = 0;
    }
}


/* Init */
int timer_init() {
    /* Disable all timers */
    TIMER8(TSTR) = 0;

    /* Set to internal clock source */
    TIMER8(TOCR) = 0;

    /* Setup the primary timer stuff */
    timer_primary_init();

    return 0;
}

/* Shutdown */
void timer_shutdown() {
    /* Shutdown primary timer stuff */
    timer_primary_shutdown();

    /* Disable all timers */
    TIMER8(TSTR) = 0;
    timer_disable_ints(TMU0);
    timer_disable_ints(TMU1);
    timer_disable_ints(TMU2);
}

/* Quick access macros */
#define PMCR_CTRL(o)  ( *((volatile uint16*)(0xff000084) + (o << 1)) )
#define PMCTR_HIGH(o) ( *((volatile uint32*)(0xff100004) + (o << 1)) )
#define PMCTR_LOW(o)  ( *((volatile uint32*)(0xff100008) + (o << 1)) )

#define PMCR_CLR        0x2000
#define PMCR_PMST       0x4000
#define PMCR_PMENABLE   0x8000
#define PMCR_RUN        0xc000
#define PMCR_PMM_MASK   0x003f

#define PMCR_CLOCK_TYPE_SHIFT 8

/* 5ns per count in 1 cycle = 1 count mode(PMCR_COUNT_CPU_CYCLES) */
#define NS_PER_CYCLE      5

/* Get a counter's current configuration */
uint16 perf_cntr_get_config(int which) {
    return PMCR_CTRL(which);
}

/* Start a performance counter */
int perf_cntr_start(int which, int mode, int count_type) {
    perf_cntr_clear(which);
    PMCR_CTRL(which) = PMCR_RUN | mode | (count_type << PMCR_CLOCK_TYPE_SHIFT);
    
    return 0;
}

/* Stop a performance counter */
int perf_cntr_stop(int which) {
    PMCR_CTRL(which) &= ~(PMCR_PMM_MASK | PMCR_PMENABLE);

    return 0;
}

/* Clears a performance counter.  Has to stop it first. */
int perf_cntr_clear(int which) {
    perf_cntr_stop(which);
    PMCR_CTRL(which) |= PMCR_CLR;

    return 0;
}

/* Returns the count value of a counter */
inline uint64 perf_cntr_count(int which) {
    return (uint64)(PMCTR_HIGH(which) & 0xffff) << 32 | PMCTR_LOW(which);
}

void timer_ns_enable() {
    perf_cntr_start(PRFC0, PMCR_ELAPSED_TIME_MODE, PMCR_COUNT_CPU_CYCLES);
}

void timer_ns_disable() {
    uint16 config = PMCR_CTRL(PRFC0);

    /* If timer is running, disable it */
    if((config & PMCR_ELAPSED_TIME_MODE)) {
        perf_cntr_clear(PRFC0);
    }
}

inline uint64 timer_ns_gettime64() {
    uint16 config = PMCR_CTRL(PRFC0);

    /* If timer is running */
    if((config & PMCR_ELAPSED_TIME_MODE)) {
        uint64 cycles = perf_cntr_count(PRFC0);
        return cycles * NS_PER_CYCLE;
    }
    else {
        uint64 micro_secs = timer_us_gettime64();
        return micro_secs * 1000;
    }
}

