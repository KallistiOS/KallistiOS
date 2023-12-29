/* KallistiOS ##version##

   arch/dreamcast/include/timer.h
   Copyright (c) 2000, 2001 Megan Potter
   Copyright (c) 2023 Falco Girgis
   
*/

/** \file    arch/timer.h
    \brief   Low-level timer functionality.
    \ingroup timers

    This file contains functions for interacting with the timer sources on the
    SH4. Many of these functions may interfere with thread operation or other
    such things, and should thus be used with caution. Basically, the only
    functionality that you might use in practice in here in normal programs is
    the gettime functions.

    \sa arch/rtc.h
    \sa arch/wdt.h

    \author Megan Potter
    \author Falco Girgis
*/

#ifndef __ARCH_TIMER_H
#define __ARCH_TIMER_H


#include <stdint.h>
#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/irq.h>

/** \defgroup timers    Timer Unit
    \brief              SH4 CPU peripheral providing timers and counters
    \ingroup            timing

    The Dreamcast's SH4 includes an on-chip Timer Unit (TMU) containing 3
    independent 32-bit channels (TMU0-TMU2). Each channel provides a
    down-counter with automatic reload and can be configured to use 1 of
    7 divider circuits for its input clock. By default, KOS uses the fastest
    input clock for each TMU channel, providing a maximum internal resolution
    of 80ns ticks.

    \warning
    Under normal circumstances, all 3 TMU channels are reserved by KOS for
    various OS-related purposes. If you need a free general-purpose interval
    timer, consider using the Watchdog Timer.

    \note
    90% of the time, you will never have a need to directly interact with this
    API, as it's mostly used as a kernel-level driver which backs other APIs.
    For example, querying for ticks, fetching the current timestamp, or putting
    a thread to sleep is typically done via the standard C, C++, or POSIX APIs.

*/

/** \defgroup tmus   Channels
    \brief           TMU channel constants
    \ingroup         timers

    The following are the constant `#define` identifiers for the 3 TMU channels.

    \warning
    All three of these channels are typically reserved and are by KOS for
    OS-related tasks.

    @{ 
*/

/** \brief  SH4 Timer Channel 0.

    \warning
    This timer is used by the kernel's scheduler for thread operation, and thus
    is off limits if you want that to work properly.
*/
#define TMU0    0

/** \brief  SH4 Timer Channel 1.

    \warning
    This timer channel is used for the timer_spin_sleep() function, which also
    backs the kthread, C, C++, and POSIX sleep functions.
*/
#define TMU1    1

/** \brief  SH4 Timer Channel 2.

    \warning
    This timer channel is used by the various gettime functions in this header.
    It also backs the standard C, C++, and POSIX date/time and clock functions.
*/
#define TMU2    2

/** @} */

/** \cond Which timer does the thread system use? */
#define TIMER_ID TMU0
/** \endcond */

/** \defgroup tmu_direct    Direct-Access
    \brief                  Low-level timer driver
    \ingroup                timers

    This API provides a low-level driver abstraction around the TMU peripheral
    and the control, counter, and reload registers of its 3 channels.

    \note
    You typically want to use the higher-level APIs associated with the
    functionality implemented by each timer channel.
*/

/** \brief   Pre-initialize a timer channel, but do not start it.
    \ingroup tmu_direct

    This function sets up a timer channel for use, but does not start it.

    \param  channel         The timer channel to set up (\ref tmus).
    \param  speed           The number of ticks per second.
    \param  interrupts      Set to 1 to receive interrupts when the timer ticks.
    \retval 0               On success.
*/
int timer_prime(int channel, uint32_t speed, int interrupts);

/** \brief   Start a timer channel.
    \ingroup tmu_direct

    This function starts a timer channel that has been initialized with
    timer_prime(), starting raising interrupts if applicable.

    \param  channel         The timer channel to start (\ref tmus).
    \retval 0               On success.
*/
int timer_start(int channel);

/** \brief   Stop a timer channel.
    \ingroup tmu_direct

    This function stops a timer channel that was started with timer_start(),
    and as a result stops interrupts coming in from the timer.

    \param  channel         The timer channel to stop (\ref tmus).
    \retval 0               On success.
*/
int timer_stop(int channel);

/** \brief   Checks whether a timer channel is running.
    \ingroup tmu_direct

    This function checks whether the given timer channel is actively counting.

    \param  channel         The timer channel to check (\ref tmus).
    \retval 0               The timer channel is stopped.
    \retval 1               The timer channel is running.
*/
int timer_running(int channel);

/** \brief   Obtain the count of a timer channel.
    \ingroup tmu_direct

    This function simply returns the count of the timer channel.

    \param  channel         The timer channel to inspect (\ref tmus).
    \return                 The timer's count.
*/
uint32_t timer_count(int channel);

/** \brief   Clear the underflow bit of a timer channel.
    \ingroup tmu_direct

    This function clears the underflow bit of a timer channel if it was set.

    \param  channel         The timer channel to clear (\ref tmus).
    \retval 0               If the underflow bit was clear (prior to calling).
    \retval 1               If the underflow bit was set (prior to calling).
*/
int timer_clear(int channel);

/** \brief   Enable high-priority timer interrupts.
    \ingroup tmu_direct

    This function enables interrupts on the specified timer.

    \param  channel        The timer channel to enable interrupts on (\ref tmus).
*/
void timer_enable_ints(int channel);

/** \brief   Disable timer interrupts.
    \ingroup tmu_direct

    This function disables interrupts on the specified timer channel.

    \param  channel         The timer channel to disable interrupts on
                            (\ref tmus).
*/
void timer_disable_ints(int channel);

/** \brief   Check whether interrupts are enabled on a timer channel.
    \ingroup tmu_direct

    This function checks whether or not interrupts are enabled on the specified
    timer channel.

    \param  channel         The timer channel to inspect (\ref tmus).
    \retval 0               If interrupts are disabled on the timer.
    \retval 1               If interrupts are enabled on the timer.
*/
int timer_ints_enabled(int channel);

/** \defgroup tmu_uptime    Uptime
    \brief                  Maintaining time since system boot.
    \ingroup                timers

    This API provides methods for querying the current system boot time or
    uptime since KOS started at various resolutions. You can use this timing
    for ticks, delta time, or frame deltas for games, profilers, or media
    decoding.

    \note
    This API is used to back the C, C++, and POSIX standard date/time
    APIs. You may wish to favor these for platform independence.

    \warning
    This API and its underlying functionality are using \ref TMU2, so any
    direct manipulation of it will interfere with the API's proper functioning.

    \note
    The highest actual tick resolution of \ref TMU2 is 80ns.
*/

/** \brief   Enable the millisecond timer.
    \ingroup tmu_uptime

    This function enables the timer used for the gettime functions. This is on
    by default. These functions use \ref TMU2 to do their work.
*/
void timer_ms_enable(void);

/** \brief   Disable the millisecond timer.
    \ingroup tmu_uptime

    This function disables the timer used for the gettime functions. Generally,
    you will not want to do this, unless you have some need to use the timer
    \ref TMU2 for something else.
*/
void timer_ms_disable(void);

/** \brief   Get the current uptime of the system (in secs and millisecs).
    \ingroup tmu_uptime

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
void timer_ms_gettime(uint32_t *secs, uint32_t *msecs);

/** \brief   Get the current uptime of the system (in milliseconds).
    \ingroup tmu_uptime

    This function retrieves the number of milliseconds since KOS was started. It
    is equivalent to calling timer_ms_gettime() and combining the number of
    seconds and milliseconds into one 64-bit value.

    \return                 The number of milliseconds since KOS started.
*/
uint64_t timer_ms_gettime64(void);

/** \brief   Get the current uptime of the system (in secs and microsecs).
    \ingroup tmu_uptime

    This function retrieves the number of seconds and microseconds since KOS was
    started.

    \param  secs            A pointer to store the number of seconds since boot
                            into.
    \param  usecs           A pointer to store the number of microseconds past
                            a second since boot.
    \note                   To get the total number of microseconds since boot,
                            calculate (*secs * 1000000) + *usecs, or use the
                            timer_us_gettime64() function.
*/
void timer_us_gettime(uint32_t *secs, uint32_t *usecs);

/** \brief   Get the current uptime of the system (in microseconds).
    \ingroup tmu_uptime

    This function retrieves the number of microseconds since KOS was started. It
    should be more precise, in theory, than timer_ms_gettime64(), but the exact
    amount of preciseness is undetermined.

    \return                 The number of microseconds since KOS started.
*/
uint64_t timer_us_gettime64(void);

/** \brief   Get the current uptime of the system (in secs and nanosecs).
    \ingroup tmu_uptime

    This function retrieves the number of seconds and nanoseconds since KOS was
    started.

    \param  secs            A pointer to store the number of seconds since boot
                            into.
    \param  nsecs           A pointer to store the number of nanoseconds past
                            a second since boot.
    \note                   To get the total number of nanoseconds since boot,
                            calculate (*secs * 1000000000) + *nsecs, or use the
                            timer_ns_gettime64() function.
*/
void timer_ns_gettime(uint32_t *secs, uint32_t *nsecs);

/** \defgroup tmu_sleep     Sleeping
    \brief                  Low-level thread sleeping
    \ingroup                timers

    This API provides the low-level functionality used to implement thread
    sleeping, used by the KOS, C, C++, and POSIX threading APIs.

    \warning
    This API and its underlying functionality are using \ref TMU1, so any
    direct manipulation of it will interfere with the API's proper functioning.
*/

/** \brief  Spin-loop sleep function.
    \ingroup tmu_sleep

    This function is meant as a very accurate delay function, even if threading
    and interrupts are disabled. It uses \ref TMU1 to sleep.

    \param  ms              The number of milliseconds to sleep.
*/
void timer_spin_sleep(int ms);

/** \defgroup tmu_primary   Primary Timer
    \brief                  Primary timer used by the kernel.
    \ingroup                timers

    This API provides a callback notification mechanism that can be hooked into
    the primary timer (TMU0). It is used by the KOS kernel for threading and
    scheduling.

    \warning
    This API and its underlying functionality are using \ref TMU0, so any
    direct manipulation of it will interfere with the API's proper functioning.
*/

/** \brief   Primary timer callback type.
    \ingroup tmu_primary

    This is the type of function which may be passed to
    timer_primary_set_callback() as the function that gets invoked
    upon interrupt.
*/
typedef void (*timer_primary_callback_t)(irq_context_t *);

/** \brief   Set the primary timer callback.
    \ingroup tmu_primary

    This function sets the primary timer callback to the specified function
    pointer.

    \warning
    Generally, you should not do this, as the threading system relies
    on the primary timer to work.

    \param  callback        The new timer callback (set to NULL to disable).
    \return                 The old timer callback.
*/
timer_primary_callback_t timer_primary_set_callback(timer_primary_callback_t callback);

/** \brief   Request a primary timer wakeup.
    \ingroup tmu_primary

    This function will wake the caller (by calling the primary timer callback)
    in approximately the number of milliseconds specified. You can only have one
    timer wakeup scheduled at a time. Any subsequently scheduled wakeups will
    replace any existing one.

    \param  millis          The number of milliseconds to schedule for.
*/
void timer_primary_wakeup(uint32_t millis);

/** \cond */
/* Init function */
int timer_init(void);

/* Shutdown */
void timer_shutdown(void);
/** \endcond */

/** \defgroup   perf_counters Performance Counters
    \brief                    SH4 CPU Performance Counter Driver
    \ingroup                  debugging

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

/** \defgroup   perf_counters_modes Modes
    \brief                          Performance Counter Modes
    \ingroup                        perf_counters

    This is the list of modes that are allowed to be passed into the perf_cntr_start()
    function, representing different things you want to count.
    
    @{
*/
/*                MODE DEFINITION                  VALUE   MEASUREMENT TYPE & NOTES */
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
    This function stops the counter before clearing it because you can't clear 
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
void timer_ns_enable(void);

/** \brief  Disable the nanosecond timer.
    \ingroup perf_counters

    This function disables the performance counter used for the timer_ns_gettime64() 
    function. Generally, you will not want to do this, unless you have some need to use 
    the counter \ref PRFC0 for something else.
*/
void timer_ns_disable(void);

/** \brief  Get the current uptime of the system (in nanoseconds).
    \ingroup perf_counters

    This function retrieves the number of nanoseconds since KOS was started.

    \return                 The number of nanoseconds since KOS started.
*/
uint64 timer_ns_gettime64(void);

__END_DECLS

#endif  /* __ARCH_TIMER_H */

