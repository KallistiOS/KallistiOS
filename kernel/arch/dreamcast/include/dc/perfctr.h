/* KallistiOS ##version##

   arch/dreamcast/include/dc/perfctr.h
   Copyright (C) 2023 Andy Barajas
   Copyright (C) 2023 Falco Girgis

*/

/** \file    dc/perfctr.h
    \brief   Low-level performance counter API
    \ingroup perf_counters

    This file contains the low-level driver for interacting with and
    utilizing the SH4's two Performance Counters, which are primarily
    used for profiling and performance tuning.

    \author MoopTheHedgehog
    \author Andy Barajas
    \author Falco Girgis
*/

#ifndef __DC_PERFCTR_H
#define __DC_PERFCTR_H

#include <stdint.h>
#include <stdbool.h>

#include <kos/cdefs.h>
__BEGIN_DECLS

/** \defgroup   perf_counters Performance Counters
    \brief      SH4 CPU Performance Counter Driver
    \ingroup    debugging

    The performance counter API exposes the SH4's hardware profiling registers,
    which consist of two different sets of independently operable 48-bit
    counters.

    @{
*/

/** \brief Identifiers for the two SH4 performance counters */
typedef enum perf_cntr {
    /** \brief  SH4 Performance Counter 0

        The first performance counter ID.

        This counter is used by KOS by default to implement the \ref
        perf_counters_timer API. Reference it for details on how to
        reconfigure it if necessary.
    */
    PRFC0,

    /** \brief  SH4 Performance Counter 1

        The second performance counter ID.

        This counter is not used anywhere internally by KOS.
    */
    PRFC1
} perf_cntr_t;

/** \brief Count clock types for the SH4 performance counters */
typedef enum perf_cntr_clock {
    /** \brief CPU Cycles

        Count CPU cycles. At 5 ns increments (for 200Mhz CPU clock), a 48-bit
        cycle counter can run continuously for 16.33 days. 
    */
    PMCR_COUNT_CPU_CYCLES,

    /** \brief  Ratio Cycles

        Count CPU/bus ratio mode cycles (where `T = C x B / 24` and `T` is
        time, `C` is count, and `B` is time of one bus cycle).

        `B` has been found to be approximately `1/99753008`, but actual time
        varies slightly. The target frequency is probably 99.75MHz.
    */
    PMCR_COUNT_RATIO_CYCLES
} perf_cntr_clock_t;


/** \brief Performance Counter Event Modes

    This is the list of modes that are allowed to be passed into the perf_cntr_start()
    function, representing different events you want to count.
*/
/*            MODE DEFINITION                  VALUE   MEASURMENT TYPE & NOTES */
typedef enum perf_cntr_event {
    PMCR_INIT_NO_MODE                         = 0x00, /**< \brief None; Just here to be complete */
    PMCR_OPERAND_READ_ACCESS_MODE             = 0x01, /**< \brief Quantity; With cache */
    PMCR_OPERAND_WRITE_ACCESS_MODE            = 0x02, /**< \brief Quantity; With cache */
    PMCR_UTLB_MISS_MODE                       = 0x03, /**< \brief Quantity */
    PMCR_OPERAND_CACHE_READ_MISS_MODE         = 0x04, /**< \brief Quantity */
    PMCR_OPERAND_CACHE_WRITE_MISS_MODE        = 0x05, /**< \brief Quantity */
    PMCR_INSTRUCTION_FETCH_MODE               = 0x06, /**< \brief Quantity; With cache */
    PMCR_INSTRUCTION_TLB_MISS_MODE            = 0x07, /**< \brief Quantity */
    PMCR_INSTRUCTION_CACHE_MISS_MODE          = 0x08, /**< \brief Quantity */
    PMCR_ALL_OPERAND_ACCESS_MODE              = 0x09, /**< \brief Quantity */
    PMCR_ALL_INSTRUCTION_FETCH_MODE           = 0x0a, /**< \brief Quantity */
    PMCR_ON_CHIP_RAM_OPERAND_ACCESS_MODE      = 0x0b, /**< \brief Quantity */
    /* No 0x0c */
    PMCR_ON_CHIP_IO_ACCESS_MODE               = 0x0d, /**< \brief Quantity */
    PMCR_OPERAND_ACCESS_MODE                  = 0x0e, /**< \brief Quantity; With cache, counts both reads and writes */
    PMCR_OPERAND_CACHE_MISS_MODE              = 0x0f, /**< \brief Quantity */
    PMCR_BRANCH_ISSUED_MODE                   = 0x10, /**< \brief Quantity; Not the same as branch taken! */
    PMCR_BRANCH_TAKEN_MODE                    = 0x11, /**< \brief Quantity */
    PMCR_SUBROUTINE_ISSUED_MODE               = 0x12, /**< \brief Quantity; Issued a BSR, BSRF, JSR, JSR/N */
    PMCR_INSTRUCTION_ISSUED_MODE              = 0x13, /**< \brief Quantity */
    PMCR_PARALLEL_INSTRUCTION_ISSUED_MODE     = 0x14, /**< \brief Quantity */
    PMCR_FPU_INSTRUCTION_ISSUED_MODE          = 0x15, /**< \brief Quantity */
    PMCR_INTERRUPT_COUNTER_MODE               = 0x16, /**< \brief Quantity */
    PMCR_NMI_COUNTER_MODE                     = 0x17, /**< \brief Quantity */
    PMCR_TRAPA_INSTRUCTION_COUNTER_MODE       = 0x18, /**< \brief Quantity */
    PMCR_UBC_A_MATCH_MODE                     = 0x19, /**< \brief Quantity */
    PMCR_UBC_B_MATCH_MODE                     = 0x1a, /**< \brief Quantity */
    /* No 0x1b-0x20 */
    PMCR_INSTRUCTION_CACHE_FILL_MODE          = 0x21, /**< \brief Cycles */
    PMCR_OPERAND_CACHE_FILL_MODE              = 0x22, /**< \brief Cycles */
    /** \brief Cycles
        For 200MHz CPU: 5ns per count in 1 cycle = 1 count mode.
        Around 417.715ps per count (increments by 12) in CPU/bus ratio mode 
    */
    PMCR_ELAPSED_TIME_MODE                    = 0x23,
    PMCR_PIPELINE_FREEZE_BY_ICACHE_MISS_MODE  = 0x24, /**< \brief Cycles */
    PMCR_PIPELINE_FREEZE_BY_DCACHE_MISS_MODE  = 0x25, /**< \brief Cycles */
    /* No 0x26 */
    PMCR_PIPELINE_FREEZE_BY_BRANCH_MODE       = 0x27, /**< \brief Cycles */
    PMCR_PIPELINE_FREEZE_BY_CPU_REGISTER_MODE = 0x28, /**< \brief Cycles */
    PMCR_PIPELINE_FREEZE_BY_FPU_MODE          = 0x29  /**< \brief Cycles */
} perf_cntr_event_t;

/** \brief  Get a performance counter's settings.

    This function returns a performance counter's settings.

    \param  counter         The performance counter (i.e, \ref PRFC0 or PRFC1).
    \param  event_mode      Pointer to fill in with the counter's event mode
    \param  clock_type      Pointer to fill in with the counter's clock type

    \retval true            The performance counter is running
    \retval false           the performance counter is stopped
*/
bool perf_cntr_config(perf_cntr_t counter, perf_cntr_event_t *event_mode,
                      perf_cntr_clock_t *clock_type);

/** \brief  Start a performance counter.

    This function starts a performance counter.

    \param  counter         The counter to start (i.e, \ref PRFC0 or PRFC1).
    \param  event_mode      Use one of the 33 event modes (pef_cntr_event_t).
    \param  clock_type      PMCR_COUNT_CPU_CYCLES or PMCR_COUNT_RATIO_CYCLES.

    \sa perf_cntr_stop(), perf_cntr_resume()
*/
void perf_cntr_start(perf_cntr_t counter, perf_cntr_event_t event_mode,
                     perf_cntr_clock_t clock_type);

/** \brief  Stop a performance counter.

    This function stops a performance counter that was started with perf_cntr_start().
    Stopping a counter retains its count. To clear the count use perf_cntr_clear().

    \param  counter           The counter to stop (i.e, \ref PRFC0 or PRFC1).

    \sa perf_cntr_clear()
*/
void perf_cntr_stop(perf_cntr_t counter);

/** \brief Resume a performance counter.

    This function resumes a stopped performance counter.

    \param counter          The counter to resume (i.e. \ref PRFC0 or PRFC1).

    \sa perf_cntr_stop()
*/
void perf_cntr_resume(perf_cntr_t counter);

/** \brief  Clear a performance counter.

    This function clears a performance counter. It resets its count to zero.
    This function stops the counter before clearing it because you cant clear
    a running counter.

    \param  counter           The counter to clear (i.e, \ref PRFC0 or PRFC1).
*/
void perf_cntr_clear(perf_cntr_t counter);

/** \brief  Obtain the count of a performance counter.

    This function simply returns the count of the counter.

    \param  counter         The counter to read (i.e, \ref PRFC0 or PRFC1).

    \return                 The counter's count.
*/
uint64_t perf_cntr_count(perf_cntr_t counter);

/** \defgroup perf_counters_timer Timer
    \brief    High-resolution performance counter-based timer API

    This API allows for using the performance counters as high-resolution
    general-purpose interval timer with 5ns ticks. It does this by configuring
    \ref PRFC0 in \ref PMCR_ELAPSED_TIME_MODE.

    \note
    This is enabled by default. To use \ref PRFC0 for something else, either
    use perf_cntr_timer_disable() or perf_cntr_start() to reconfigure it for
    something else. When disabled, the timer will simply fall through to use
    timer_ns_gettime64() from the timer driver, decreasing the resolution of
    each tick to 80ns.

    \warning
    The performance counter timer is only counting \a active CPU cycles. This
    means that when KOS's thread scheduler uses the "sleep" instruction,
    putting the CPU to sleep, these counters cease to record elapsed time.
    Because of this, they should only be used to measure small deltas that
    are not across frames, when you want real wall time rather than active
    CPU time.

    \sa timers

    @{
*/

/** \brief  Enable the nanosecond timer.

    This function enables the performance counter used for the perf_cntr_timer_ns()
    function. 

    \note
    This is on by default. The function uses \ref PRFC0 to do the work.

    \warning
    The performance counters are only counting \a active CPU cycles while in
    this mode. This is analogous to providing you with the CPU time of your
    application, not the actual wall-time or monotonic clock, as it ceases
    to count when the kernel puts the CPU to sleep.
*/
void perf_cntr_timer_enable(void);

/** \brief  Disable the nanosecond timer.

    This function disables the performance counter used for the
    perf_cntr_timer_ns() function.

    \note
    Generally, you will not want to do this, unless you have some need to use
    the counter \ref PRFC0 for something else.
*/
void perf_cntr_timer_disable(void);

/** \brief Check whether the nanosecond timer is enabled.

    Queries the configuration of \ref PRFC0 to check whether it's
    currently acting as the nanosecond timer.

    \note
    Even when it's not, perf_cntr_timer_ns() will still gracefully fall-through
    to using the timer_ns_gettime64(), which decreases the resolution of each
    tick to 80ns.

    \retval true    The nanosecond timer is configured and running
    \retval false   The nanosecond timer is not configured and/or isn't
                    running
*/
bool perf_cntr_timer_enabled(void);

/** \brief  Gets elapsed CPU time (in nanoseconds)

    This function retrieves the total amount of \a active CPU time since
    perf_cntr_timer_enabled() was called.

    \note
    It's called by default when KOS initializes, so unless you reinitialize it
    later on, this should be the total CPU time since KOS booted up.

    \return                 The number of nanoseconds of active CPU time since
                            the timer was enabled.
*/
uint64_t perf_cntr_timer_ns(void);

/** @} */

/** @} */

__END_DECLS

#endif /* __DC_PERFCTR_H */
