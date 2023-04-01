/* KallistiOS ##version##

   arch/dreamcast/include/timer.h
   (c)2000-2001 Megan Potter

*/

/** \file   arch/timer.h
    \brief  Low-level timer functionality.

    This file contains functions for interacting with the timer sources on the
    SH4. Many of these functions may interfere with thread operation or other
    such things, and should thus be used with caution. Basically, the only
    functionality that you might use in practice in here in normal programs is
    the gettime functions.

    \author Megan Potter
*/

#ifndef __ARCH_TIMER_H
#define __ARCH_TIMER_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <arch/irq.h>

/* Timer sources -- we get four on the SH4 */

/** \brief  SH4 Timer 0.

    This timer is used for thread operation, and thus is off limits if you want
    that to work properly.
*/
#define TMU0    0

/** \brief  SH4 Timer 1.

    This timer is used for the timer_spin_sleep() function.
*/
#define TMU1    1

/** \brief  SH4 Timer 2.

    This timer is used by the various gettime functions in this header.
*/
#define TMU2    2

/** \brief  SH4 Watchdog Timer.

    KallistiOS does not currently support using this timer.
*/
#define WDT     3

/** \brief  Which timer does the thread system use? */
#define TIMER_ID TMU0

/** \brief  Pre-initialize a timer, but do not start it.

    This function sets up a timer for use, but does not start it.

    \param  which           The timer to set up (i.e, \ref TMU0).
    \param  speed           The number of ticks per second.
    \param  interrupts      Set to 1 to receive interrupts when the timer ticks.
    \retval 0               On success.
*/
int timer_prime(int which, uint32 speed, int interrupts);

/** \brief  Start a timer.

    This function starts a timer that has been initialized with timer_prime(),
    starting raising interrupts if applicable.

    \param  which           The timer to start (i.e, \ref TMU0).
    \retval 0               On success.
*/
int timer_start(int which);

/** \brief  Stop a timer.

    This function stops a timer that was started with timer_start(), and as a
    result stops interrupts coming in from the timer.

    \param  which           The timer to stop (i.e, \ref TMU0).
    \retval 0               On success.
*/
int timer_stop(int which);

/** \brief  Obtain the count of a timer.

    This function simply returns the count of the timer.

    \param  which           The timer to inspect (i.e, \ref TMU0).
    \return                 The timer's count.
*/
uint32 timer_count(int which);

/** \brief  Clear the underflow bit of a timer.

    This function clears the underflow bit of a timer if it was set.

    \param  which           The timer to inspect (i.e, \ref TMU0).
    \retval 0               If the underflow bit was clear (prior to calling).
    \retval 1               If the underflow bit was set (prior to calling).
*/
int timer_clear(int which);

/** \brief  Spin-loop sleep function.

    This function is meant as a very accurate delay function, even if threading
    and interrupts are disabled. It uses \ref TMU1 to sleep.

    \param  ms              The number of milliseconds to sleep.
*/
void timer_spin_sleep(int ms);

/** \brief  Enable high-priority timer interrupts.

    This function enables interrupts on the specified timer.

    \param  which           The timer to enable interrupts on (i.e, \ref TMU0).
*/
void timer_enable_ints(int which);

/** \brief  Disable timer interrupts.

    This function disables interrupts on the specified timer.

    \param  which           The timer to disable interrupts on (i.e, \ref TMU0).
*/
void timer_disable_ints(int which);

/** \brief  Check whether interrupts are enabled on a timer.

    This function checks whether or not interrupts are enabled on the specified
    timer.

    \param  which           The timer to inspect (i.e, \ref TMU0).
    \retval 0               If interrupts are disabled on the timer.
    \retval 1               If interrupts are enabled on the timer.
*/
int timer_ints_enabled(int which);

/** \brief  Enable the millisecond timer.

    This function enables the timer used for the gettime functions. This is on
    by default. These functions use \ref TMU2 to do their work.
*/
void timer_ms_enable();

/** \brief  Disable the millisecond timer.

    This function disables the timer used for the gettime functions. Generally,
    you will not want to do this, unless you have some need to use the timer
    \ref TMU2 for something else.
*/
void timer_ms_disable();

/** \brief  Get the current uptime of the system.

    This function retrieves the number of seconds and milliseconds since KOS was
    started.

    \param  secs            A pointer to store the number of seconds since boot
                            into.
    \param  msecs           A pointer to store the number of milliseconds past
                            a second since boot.
    \note                   To get the total number of milliseconds since boot,
                            calculate (*secs * 1000) + *msecs, or use the
                            timer_ms_gettime64() function.
*/
void timer_ms_gettime(uint32 *secs, uint32 *msecs);

/** \brief  Get the current uptime of the system (in milliseconds).

    This function retrieves the number of milliseconds since KOS was started. It
    is equivalent to calling timer_ms_gettime() and combining the number of
    seconds and milliseconds into one 64-bit value.

    \return                 The number of milliseconds since KOS started.
*/
uint64 timer_ms_gettime64();

/** \brief  Get the current uptime of the system (in microseconds).

    This function retrieves the number of microseconds since KOS was started. It
    should be more precise, in theory, than timer_ms_gettime64(), but the exact
    amount of preciseness is undetermined.

    \return                 The number of microseconds since KOS started.
*/
uint64 timer_us_gettime64();

/** \brief  Primary timer callback type. */
typedef void (*timer_primary_callback_t)(irq_context_t *);

/** \brief  Set the primary timer callback.

    This function sets the primary timer callback to the specified function
    pointer. Generally, you should not do this, as the threading system relies
    on the primary timer to work.

    \param  callback        The new timer callback (set to NULL to disable).
    \return                 The old timer callback.
*/
timer_primary_callback_t timer_primary_set_callback(timer_primary_callback_t callback);

/** \brief  Request a primary timer wakeup.

    This function will wake the caller (by calling the primary timer callback)
    in approximately the number of milliseconds specified. You can only have one
    timer wakeup scheduled at a time. Any subsequently scheduled wakeups will
    replace any existing one.

    \param  millis          The number of milliseconds to schedule for.
*/
void timer_primary_wakeup(uint32 millis);

/* \cond */
/* Init function */
int timer_init();

/* Shutdown */
void timer_shutdown();
/* \endcond */

/** \defgroup   perf_counters Performance Counters
    The performance counter API exposes the SH4's hardware profiling registers, 
    which consist of two different sets of independently operable 64-bit 
    counters.  
*/

/** \brief  SH4 Performance Counter.
    \ingroup perf_counters

    This counter is used by the ns_gettime function in this header.
*/
#define PRFC0   0

/** \brief  SH4 Performance Counter.
    \ingroup perf_counters

    A counter that is not used by KOS.
*/
#define PRFC1   1

/** \brief  CPU Cycles Count Type.
    \ingroup perf_counters

    Count cycles. At 5 ns increments, a 48-bit cycle counter can 
    run continuously for 16.33 days.
*/
#define PMCR_COUNT_CPU_CYCLES 0

/** \brief  Ratio Cycles Count Type.
    \ingroup perf_counters

    CPU/bus ratio mode where cycles (where T = C x B / 24 and T is time, 
    C is count, and B is time of one bus cycle).
*/
#define PMCR_COUNT_RATIO_CYCLES 1

/** \defgroup   perf_counters_modes Performance Counter Modes
    This is the list of modes that are allowed to be passed into the perf_cntr_start()
    function, representing different things you want to count.
    \ingroup perf_counters
    @{
*/
/*                MODE DEFINITION                  VALUE   MEASURMENT TYPE & NOTES */
#define PMCR_INIT_NO_MODE                           0x00 /**< \brief None; Just here to be complete */
#define PMCR_OPERAND_READ_ACCESS_MODE               0x01 /**< \brief Quantity; With cache */
#define PMCR_OPERAND_WRITE_ACCESS_MODE              0x02 /**< \brief Quantity; With cache */
#define PMCR_UTLB_MISS_MODE                         0x03 /**< \brief Quantity */
#define PMCR_OPERAND_CACHE_READ_MISS_MODE           0x04 /**< \brief Quantity */
#define PMCR_OPERAND_CACHE_WRITE_MISS_MODE          0x05 /**< \brief Quantity */
#define PMCR_INSTRUCTION_FETCH_MODE                 0x06 /**< \brief Quantity; With cache */
#define PMCR_INSTRUCTION_TLB_MISS_MODE              0x07 /**< \brief Quantity */
#define PMCR_INSTRUCTION_CACHE_MISS_MODE            0x08 /**< \brief Quantity */
#define PMCR_ALL_OPERAND_ACCESS_MODE                0x09 /**< \brief Quantity */
#define PMCR_ALL_INSTRUCTION_FETCH_MODE             0x0a /**< \brief Quantity */
#define PMCR_ON_CHIP_RAM_OPERAND_ACCESS_MODE        0x0b /**< \brief Quantity */
/* No 0x0c */
#define PMCR_ON_CHIP_IO_ACCESS_MODE                 0x0d /**< \brief Quantity */
#define PMCR_OPERAND_ACCESS_MODE                    0x0e /**< \brief Quantity; With cache, counts both reads and writes */
#define PMCR_OPERAND_CACHE_MISS_MODE                0x0f /**< \brief Quantity */
#define PMCR_BRANCH_ISSUED_MODE                     0x10 /**< \brief Quantity; Not the same as branch taken! */
#define PMCR_BRANCH_TAKEN_MODE                      0x11 /**< \brief Quantity */
#define PMCR_SUBROUTINE_ISSUED_MODE                 0x12 /**< \brief Quantity; Issued a BSR, BSRF, JSR, JSR/N */
#define PMCR_INSTRUCTION_ISSUED_MODE                0x13 /**< \brief Quantity */
#define PMCR_PARALLEL_INSTRUCTION_ISSUED_MODE       0x14 /**< \brief Quantity */
#define PMCR_FPU_INSTRUCTION_ISSUED_MODE            0x15 /**< \brief Quantity */
#define PMCR_INTERRUPT_COUNTER_MODE                 0x16 /**< \brief Quantity */
#define PMCR_NMI_COUNTER_MODE                       0x17 /**< \brief Quantity */
#define PMCR_TRAPA_INSTRUCTION_COUNTER_MODE         0x18 /**< \brief Quantity */
#define PMCR_UBC_A_MATCH_MODE                       0x19 /**< \brief Quantity */
#define PMCR_UBC_B_MATCH_MODE                       0x1a /**< \brief Quantity */
/* No 0x1b-0x20 */
#define PMCR_INSTRUCTION_CACHE_FILL_MODE            0x21 /**< \brief Cycles */
#define PMCR_OPERAND_CACHE_FILL_MODE                0x22 /**< \brief Cycles */
#define PMCR_ELAPSED_TIME_MODE                      0x23 /**< \brief Cycles; For 200MHz CPU: 5ns per count in 1 cycle = 1 count mode, or around 417.715ps per count (increments by 12) in CPU/bus ratio mode */
#define PMCR_PIPELINE_FREEZE_BY_ICACHE_MISS_MODE    0x24 /**< \brief Cycles */
#define PMCR_PIPELINE_FREEZE_BY_DCACHE_MISS_MODE    0x25 /**< \brief Cycles */
/* No 0x26 */
#define PMCR_PIPELINE_FREEZE_BY_BRANCH_MODE         0x27 /**< \brief Cycles */
#define PMCR_PIPELINE_FREEZE_BY_CPU_REGISTER_MODE   0x28 /**< \brief Cycles */
#define PMCR_PIPELINE_FREEZE_BY_FPU_MODE            0x29 /**< \brief Cycles */
/** @} */


/** \brief  Get a performance counter's settings.
    \ingroup perf_counters

    This function returns a performance counter's settings.

    \param  which           The performance counter (i.e, \ref PRFC0 or PRFC1).
    \retval 0               On success.
*/
uint16 perf_cntr_get_config(int which);

/** \brief  Start a performance counter.
    \ingroup perf_counters

    This function starts a performance counter

    \param  which           The counter to start (i.e, \ref PRFC0 or PRFC1).
    \param  mode            Use one of the 33 modes listed above.
    \param  count_type      PMCR_COUNT_CPU_CYCLES or PMCR_COUNT_RATIO_CYCLES.
    \retval 0               On success.
*/
int perf_cntr_start(int which, int mode, int count_type);

/** \brief  Stop a performance counter.
    \ingroup perf_counters

    This function stops a performance counter that was started with perf_cntr_start().
    Stopping a counter retains its count. To clear the count use perf_cntr_clear().

    \param  which           The counter to stop (i.e, \ref PRFC0 or PRFC1).
    \retval 0               On success.
*/
int perf_cntr_stop(int which);

/** \brief  Clear a performance counter.
    \ingroup perf_counters

    This function clears a performance counter. It resets its count to zero.
    This function stops the counter before clearing it because you cant clear 
    a running counter.

    \param  which           The counter to clear (i.e, \ref PRFC0 or PRFC1).
    \retval 0               On success.
*/
int perf_cntr_clear(int which);

/** \brief  Obtain the count of a performance counter.
    \ingroup perf_counters

    This function simply returns the count of the counter.

    \param  which           The counter to read (i.e, \ref PRFC0 or PRFC1).
    \return                 The counter's count.
*/
uint64 perf_cntr_count(int which);

/** \brief  Enable the nanosecond timer.
    \ingroup perf_counters

    This function enables the performance counter used for the timer_ns_gettime64() 
    function. This is on by default. The function uses \ref PRFC0 to do the work.
*/
void timer_ns_enable();

/** \brief  Disable the nanosecond timer.
    \ingroup perf_counters

    This function disables the performance counter used for the timer_ns_gettime64() 
    function. Generally, you will not want to do this, unless you have some need to use 
    the counter \ref PRFC0 for something else.
*/
void timer_ns_disable();

/** \brief  Get the current uptime of the system (in nanoseconds).
    \ingroup perf_counters

    This function retrieves the number of nanoseconds since KOS was started.

    \return                 The number of nanoseconds since KOS started.
*/
uint64 timer_ns_gettime64();

__END_DECLS

#endif  /* __ARCH_TIMER_H */

