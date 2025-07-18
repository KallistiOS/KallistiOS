/* KallistiOS ##version##

   timer.c
   Copyright (C) 2000, 2001, 2002 Megan Potter
   Copyright (C) 2023 Falco Girgis
   Copyright (C) 2023, 2024 Paul Cercueil <paul@crapouillou.net>
*/

#include <assert.h>
#include <stdio.h>

#include <arch/arch.h>
#include <arch/timer.h>
#include <arch/irq.h>
#include <kos/regfield.h>

/* Register access macros */
#define TIMER8(o)   ( *((volatile uint8_t  *)(TIMER_BASE + (o))) )
#define TIMER16(o)  ( *((volatile uint16_t *)(TIMER_BASE + (o))) )
#define TIMER32(o)  ( *((volatile uint32_t *)(TIMER_BASE + (o))) )

/* Register base address */
#define TIMER_BASE 0xffd80000 

/* Register offsets */
#define TOCR    0x00    /* Timer Output Control Register */
#define TSTR    0x04    /* Timer Start Register */
#define TCOR0   0x08    /* Timer Constant Register 0 */
#define TCNT0   0x0c    /* Timer Counter Register 0 */
#define TCR0    0x10    /* Timer Control Register 0 */
#define TCOR1   0x14    /* Timer Constant Register 1 */
#define TCNT1   0x18    /* Timer Counter Register 1 */
#define TCR1    0x1c    /* Timer Control Register 1 */
#define TCOR2   0x20    /* Timer Constant Register 2 */
#define TCNT2   0x24    /* Timer Counter Register 2 */
#define TCR2    0x28    /* Timer Control Register 2 */
#define TCPR2   0x2c    /* Timer Input Capture */

/* Timer Start Register fields */
#define STR2    2   /* TCNT2 Counter Start */
#define STR1    1   /* TCNT1 Counter Start */
#define STR0    0   /* TCNT0 Counter Start */

/* Timer Control Register fields */
#define ICPF    BIT(9)          /* Input Capture Interrupt Flag (TMU2 only) */
#define UNF     BIT(8)          /* Underflow Flag */
#define ICPE    GENMASK(7, 6)   /* Input Capture Control (TMU2 only) */
#define UNIE    BIT(5)          /* Underflow Interrupt Control */
#define CKEG    GENMASK(4, 3)   /* Clock Edge */
#define TPSC    GENMASK(2, 0)   /* Timer Prescalar */

/* Clock divisor value for each TPSC value. */
#define TDIV(div)   (4 << (2 * div))

/* Timer Prescalar TPSC values (Peripheral clock divided by N) */
typedef enum PCK_DIV { 
    PCK_DIV_4,      /* Pck/4    => 80ns */
    PCK_DIV_16,     /* Pck/16   => 320ns*/
    PCK_DIV_64,     /* Pck/64   => 1280ns*/
    PCK_DIV_256,    /* Pck/256  => 5120ns*/
    PCK_DIV_1024    /* Pck/1024 => 20480ns*/
} PCK_DIV;

/* Timer TPSC values (4 for highest resolution timings) */
#define TIMER_TPSC      PCK_DIV_4
/* Timer IRQ priority levels (0-15) */
#define TIMER_PRIO      15

/* Peripheral clock rate (~49.9 Mhz).
 * The main clock is not exactly 200 MHz, and has been measured at
 * 199499520 Hz. The peripheral clock is a quarter of that. */
#define TIMER_PCK       (199499520 / 4)

/* Timer registers, indexed by Timer ID. */
static const unsigned tcors[] = { TCOR0, TCOR1, TCOR2 };
static const unsigned tcnts[] = { TCNT0, TCNT1, TCNT2 };
static const unsigned tcrs[] = { TCR0, TCR1, TCR2 };

/* Apply timer configuration to registers. */
static int timer_prime_apply(int which, uint32_t count, int interrupts) { 
    assert(which <= TMU2);

    TIMER32(tcnts[which]) = count;
    TIMER32(tcors[which]) = count; 

    TIMER16(tcrs[which]) = TIMER_TPSC;

    /* Enable IRQ generation plus unmask and set priority */
    if(interrupts) {
        TIMER16(tcrs[which]) |= UNIE;
        timer_enable_ints(which);
    }

    return 0;
}

/* Pre-initialize a timer; set values but don't start it.
   "speed" is the number of desired ticks per second. */
int timer_prime(int which, uint32_t speed, int interrupts) {
    /* Initialize counters; formula is P0/(tps*div) */
    const uint32_t cd = TIMER_PCK / (speed * TDIV(TIMER_TPSC));

    return timer_prime_apply(which, cd, interrupts);
}

/* Works like timer_prime, but takes an interval in milliseconds
   instead of a rate. Used by the primary timer stuff. */
static int timer_prime_wait(int which, uint32_t millis, int interrupts) {
    /* Calculate the countdown, formula is P0 * millis/div*1000. We
       rearrange the math a bit here to avoid integer overflows. */
    const uint32_t cd = (TIMER_PCK / TDIV(TIMER_TPSC)) * millis / 1000;

    return timer_prime_apply(which, cd, interrupts);
}

/* Start a timer -- starts it running (and interrupts if applicable) */
int timer_start(int which) {
    assert(which <= TMU2);

    TIMER8(TSTR) |= BIT(which);
    return 0;
}

/* Stop a timer -- and disables its interrupt */
int timer_stop(int which) {
    assert(which <= TMU2);

    timer_disable_ints(which);

    /* Stop timer */
    TIMER8(TSTR) &= ~BIT(which);

    return 0;
}

int timer_running(int which) {
    assert(which <= TMU2);

    return !!(TIMER8(TSTR) & BIT(which));
}

/* Returns the count value of a timer */
uint32_t timer_count(int which) {
    assert(which <= TMU2);

    return TIMER32(tcnts[which]);
}

/* Clears the timer underflow bit and returns what its value was */
int timer_clear(int which) {
    uint16_t value;

    assert(which <= TMU2);
    value = TIMER16(tcrs[which]);

    TIMER16(tcrs[which]) &= ~UNF;
    return !!(value & UNF);
}

/* Spin-loop kernel sleep func: uses the secondary timer in the
   SH-4 to very accurately delay even when interrupts are disabled */
void timer_spin_sleep(int ms) {
    timer_prime(TMU1, 1000, 0);
    timer_clear(TMU1);
    timer_start(TMU1);

    while(ms > 0) {
        while(!(TIMER16(tcrs[TMU1]) & UNF))
            ;

        timer_clear(TMU1);
        ms--;
    }

    timer_stop(TMU1);
}

void timer_spin_delay_ns(unsigned short ns) {
    uint64_t timeout = timer_ns_gettime64() + ns;

    /* Note that we don't actually care about the counter overflowing.
       Nobody will run their Dreamcast straight for 585 years. */
    while(timer_ns_gettime64() < timeout);
}

void timer_spin_delay_us(unsigned short us) {
    uint64_t timeout = timer_us_gettime64() + us;

    /* Note that we don't actually care about the counter overflowing.
       Nobody will run their Dreamcast straight for 584942 years. */
    while(timer_us_gettime64() < timeout);
}

/* Enable timer interrupts; needs to move to irq.c sometime. */
void timer_enable_ints(int which) {
    irq_set_priority(IRQ_SRC_TMU0 - which, TIMER_PRIO);
}

/* Disable timer interrupts; needs to move to irq.c sometime. */
void timer_disable_ints(int which) {
    irq_set_priority(IRQ_SRC_TMU0 - which, IRQ_PRIO_MASKED);
}

/* Check whether ints are enabled */
int timer_ints_enabled(int which) {
    return irq_get_priority(IRQ_SRC_TMU0 - which) > 0;
}

/* Seconds elapsed (since KOS startup), updated from the TMU2 underflow ISR */
static volatile uint32_t timer_ms_counter = 0; 
/* Max counter value (used as TMU2 reload), to target a 1 second interval */
static          uint32_t timer_ms_countdown;   

/* TMU2 interrupt handler, called every second. Simply updates our
   running second counter and clears the underflow flag. */
static void timer_ms_handler(irq_t source, irq_context_t *context, void *data) {
    (void)source;
    (void)context;
    (void)data;

    timer_ms_counter++;

    /* Clear overflow bit so we can check it when returning time */
    TIMER16(tcrs[TMU2]) &= ~UNF;
}

void timer_ms_enable(void) {
    irq_set_handler(EXC_TMU2_TUNI2, timer_ms_handler, NULL);
    timer_prime(TMU2, 1, 1);
    timer_ms_countdown = timer_count(TMU2);
    timer_clear(TMU2);
    timer_start(TMU2);
}

void timer_ms_disable(void) {
    timer_stop(TMU2);
    timer_disable_ints(TMU2);
}

/* Internal structure used to hold timer values in seconds + ticks. */
typedef struct timer_value {
    uint32_t secs, ticks;
} timer_val_t;

/* Generic function for retrieving the current time maintained by TMU2. 
   Returns the total amount of time that has elapsed since KOS has been
   initialized by using a LUT of precomputed, scaled timing values (tns)
   plus a shift for optimized division. */
static timer_val_t timer_getticks(const uint32_t *tns, uint32_t shift) {
    uint32_t secs, unf1, unf2, counter1, counter2, delta, ticks;
    uint16_t tmu2;
    
    do {
        /* Read the underflow flag twice, and the counter twice.
           - If both flags are set, it's just unrealistic that one
             second elapsed between the two reads, therefore we can
             assume that the interrupt did not fire yet, and both
             the timer value and the computation of "secs" are valid.
           - If one underflow flag is set, and the other is not,
             the timer value or the "secs" value cannot be trusted;
             loop and try again.
           - If both flags are cleared, either the timer did not
             underflow, or it did but the interrupt handler was quick
             enough to clear the flag, in which case the computation
             of "secs" may be wrong. We can check that by reading
             the timer value again, and if it's above the previous
             value, the timer underflowed and we have to try again.

           This complex setup avoids the issue where the timer
           underflows between the moment where you compute the
           seconds value, and the moment where you read the timer.
           It also does not require the interrupts to be masked. */
        counter1 = TIMER32(tcnts[TMU2]);
        tmu2 = TIMER16(tcrs[TMU2]);
        unf1 = !!(tmu2 & UNF);
        secs = timer_ms_counter + unf1;

        counter2 = TIMER32(tcnts[TMU2]);
        tmu2 = TIMER16(tcrs[TMU2]);
        unf2 = !!(tmu2 & UNF);
    } while (__unlikely(unf1 != unf2 || counter1 < counter2));

    delta = timer_ms_countdown - counter2;

    /* We have to do the elapsed time calculations as a 64-bit unsigned
    integer, otherwise when using the fastest clock speed for timers,
    this value will very quickly overflow mid-expression, before the
    final division. */
    ticks = ((uint64_t)delta * tns[tmu2 & TPSC]) >> shift;

    return (timer_val_t){ .secs = secs, .ticks = ticks, };
}

/* Millisecond timer */
static const uint32_t tns_values_ms[] = {
    /* 80, 320, 1280, 5120, 20480
       each multiplied by (1 << 37) / (1000 * 1000) */
    10995116, 43980465, 175921860, 703687442, 2814749767
};

void timer_ms_gettime(uint32_t *secs, uint32_t *msecs) {
    const timer_val_t val = timer_getticks(tns_values_ms, 37);

    if(secs)  *secs = val.secs;
    if(msecs) *msecs = val.ticks;
}

uint64_t timer_ms_gettime64(void) {
   const timer_val_t val = timer_getticks(tns_values_ms, 37);

    return (uint64_t)val.secs * 1000ull + (uint64_t)val.ticks;
}

/* Microsecond timer */
static const uint32_t tns_values_us[] = {
    /* 80, 320, 1280, 5120, 20480,
       each multiplied by (1 << 27) / 1000 */
    10737418, 42949673, 171798692, 687194767, 2748779069,
};

void timer_us_gettime(uint32_t *secs, uint32_t *usecs) {
    const timer_val_t val = timer_getticks(tns_values_us, 27);

    if(secs)  *secs = val.secs;
    if(usecs) *usecs = val.ticks;
}

uint64_t timer_us_gettime64(void) {
   const timer_val_t val = timer_getticks(tns_values_us, 27);

    return (uint64_t)val.secs * 1000000ull + (uint64_t)val.ticks;
}

/* Nanosecond timer */
static const uint32_t tns_values_ns[] = {
    80, 320, 1280, 5120, 20480,
};

void timer_ns_gettime(uint32_t *secs, uint32_t *nsecs) { 
    const timer_val_t val = timer_getticks(tns_values_ns, 0);

    if(secs)  *secs = val.secs;
    if(nsecs) *nsecs = val.ticks;
}

uint64_t timer_ns_gettime64(void) {
   const timer_val_t val = timer_getticks(tns_values_ns, 0);

    return (uint64_t)val.secs * 1000000000ull + (uint64_t)val.ticks;
}

/* Primary kernel timer. What we'll do here is handle actual timer IRQs
   internally, and call the callback only after the appropriate number of
   millis has passed. For the DC you can't have timers spaced out more
   than about one second, so we emulate longer waits with a counter. */
static timer_primary_callback_t tp_callback;
static uint32_t tp_ms_remaining;

/* IRQ handler for the primary timer interrupt. */
static void tp_handler(irq_t src, irq_context_t *cxt, void *data) {
    (void)src;
    (void)data;

    /* Are we at zero? */
    if(tp_ms_remaining == 0) {
        /* Disable any further timer events. The callback may
           re-enable them of course. */
        timer_stop(TMU0);
        timer_disable_ints(TMU0);

        /* Call the callback, if any */
        if(tp_callback)
            tp_callback(cxt);
    } 
    /* Do we have less than a second remaining? */
    else if(tp_ms_remaining < 1000) {
        /* Schedule a "last leg" timer. */
        timer_stop(TMU0);
        timer_prime_wait(TMU0, tp_ms_remaining, 1);
        timer_clear(TMU0);
        timer_start(TMU0);
        tp_ms_remaining = 0;
    } 
    /* Otherwise, we're just counting down. */
    else {
        tp_ms_remaining -= 1000;
    }
}

/* Enable / Disable primary kernel timer */
static void timer_primary_init(void) {
    /* Clear out our vars */
    tp_callback = NULL;

    /* Clear out TMU0 and get ready for wakeups */
    irq_set_handler(EXC_TMU0_TUNI0, tp_handler, NULL);
    timer_clear(TMU0);
}

static void timer_primary_shutdown(void) {
    timer_stop(TMU0);
    timer_disable_ints(TMU0);
    irq_set_handler(EXC_TMU0_TUNI0, NULL, NULL);
}

timer_primary_callback_t timer_primary_set_callback(timer_primary_callback_t cb) {
    timer_primary_callback_t cbold = tp_callback;
    tp_callback = cb;
    return cbold;
}

void timer_primary_wakeup(uint32_t millis) {
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
int timer_init(void) {
    /* Disable all timers */
    TIMER8(TSTR) = 0;

    /* Set to internal clock source */
    TIMER8(TOCR) = 0;

    /* Setup the primary timer stuff */
    timer_primary_init();

    return 0;
}

/* Shutdown */
void timer_shutdown(void) {
    /* Shutdown primary timer stuff */
    timer_primary_shutdown();

    /* Disable all timers */
    TIMER8(TSTR) = 0;
    timer_disable_ints(TMU0);
    timer_disable_ints(TMU1);
    timer_disable_ints(TMU2);
}
