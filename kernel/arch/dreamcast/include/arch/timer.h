/* KallistiOS ##version##

   arch/dreamcast/include/timer.h
   Copyright (c) 2000, 2001 Megan Potter
   Copyright (c) 2023, 2024 Falco Girgis
   
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

    \todo
    - Remove spin_loop_sleep() legacy compatibility macro.

    \author Megan Potter
    \author Falco Girgis
*/

#ifndef __ARCH_TIMER_H
#define __ARCH_TIMER_H

#include <stdint.h>
#include <stdbool.h>
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
    Under normal circumstances, 2 TMU channels are reserved by KOS for
    various OS-related purposes. If you need a free general-purpose interval
    timer, consider using the Watchdog Timer.

    \note
    90% of the time, you will never have a need to directly interact with this
    API, as it's mostly used as a kernel-level driver which backs other APIs.
    For example, querying for ticks, fetching the current timestamp, or putting
    a thread to sleep is typically done via the standard C, C++, or POSIX APIs.

    @{
*/

/** \brief           TMU channel identifiers

    The following are the identifiers for the 3 TMU channels.

    \warning
    All three of these channels are typically reserved and are by KOS for
    OS-related tasks.
*/
typedef enum timer_channel {
    /** \brief  SH4 Timer Channel 0.

        \warning
        This timer channel is used by the kernel's scheduler for thread
        operation, and thus is off limits if you want that to work properly.
    */
    TMU0 = 0,

    /** \brief  SH4 Timer Channel 1.

        This timer is currently free to use by an application or driver.
    */
    TMU1 = 1,

    /** \brief  SH4 Timer Channel 2.

        \warning
        This timer channel is used by the various gettime functions in this
        header. It also backs the standard C, C++, and POSIX date/time and
        clock functions.
    */
    TMU2 = 2
} timer_channel_t;

/** \cond Which timer channel does the thread system use? */
#define TIMER_ID TMU0
/** \endcond */

/** \defgroup tmu_direct    Direct-Access
    \brief                  Low-level timer driver

    This API provides a low-level driver abstraction around the TMU peripheral
    and the control, counter, and reload registers of its 3 channels.

    \note
    You typically want to use the higher-level APIs associated with the
    functionality implemented by each timer unit.

    @{
*/

/** \brief   Pre-initialize a timer channel, but do not start it.

    This function sets up a timer channel for use, but does not start it.

    \param  channel         The timer channel to set up.
    \param  speed           The number of ticks per second.
    \param  interrupts      Set to true to receive interrupts upon timeout.

    \retval 0               On success.
*/
int timer_prime(timer_channel_t channel, uint32_t speed, bool interrupts);

/** \brief   Start a timer unit.

    This function starts a timer channel that has been initialized with
    timer_prime(), starting raising interrupts if applicable.

    \param  channel         The timer channel to start.
    \retval 0               On success.

    \sa timer_stop()
*/
int timer_start(timer_channel_t channel);

/** \brief   Stop a timer channel.

    This function stops a timer channel that was started with timer_start(),
    and as a result stops interrupts coming in from the timer.

    \param  channel         The timer channel to stop.
    \retval 0               On success.

    \sa timer_start()
*/
int timer_stop(timer_channel_t channel);

/** \brief   Checks whether a timer channel is running.

    This function checks whether the given timer channel is actively counting.

    \param  channe          The timer channel to check.
    \retval false           The timer channel is stopped.
    \retval true            The timer channel is running.

    \sa timer_start(), timer_stop()
*/
bool timer_running(timer_channel_t channel);

/** \brief   Obtain the counter value of a timer channel.

    This function simply returns the current counter value of the timer
    channel.

    \param  channel         The timer channel to inspect.
    \return                 The timer's count.
*/
uint32_t timer_count(timer_channel_t channel);

/** \brief   Clear the underflow bit of a timer channel.

    This function clears the underflow bit of a timer channel if it was set.

    \param  channel         The timer channel to clear.
    \retval 0               If the underflow bit was clear (prior to calling).
    \retval 1               If the underflow bit was set (prior to calling).
*/
int timer_clear(timer_channel_t channel);

/** \brief   Enable high-priority timer interrupts.

    This function enables interrupts on the specified timer channel.

    \param  channel           The timer channel to enable interrupts on.

    \sa timmer_disable_ints()
*/
void timer_enable_ints(timer_channel_t channel);

/** \brief   Disable timer interrupts.

    This function disables interrupts on the specified timer channel.

    \param  channel          The timer channel to disable interrupts on.

    \sa timer_enable_ints()
*/
void timer_disable_ints(timer_channel_t channel);

/** \brief   Check whether interrupts are enabled on a timer channel.

    This function checks whether or not interrupts are enabled on the specified
    timer channel.

    \param  channel         The timer channel to inspect.
    \retval false           If interrupts are disabled on the timer.
    \retval true            If interrupts are enabled on the timer.

    \sa timer_enable_ints(), timer_disable_ints()
*/
bool timer_ints_enabled(timer_channel_t channel);

/** @} */

/** \defgroup tmu_uptime    Uptime
    \brief                  Maintaining time since system boot.

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

    @{
*/

/** \brief   Enable the millisecond timer.

    This function enables the timer used for the gettime functions. This is on
    by default. These functions use \ref TMU2 to do their work.

    \sa timer_ms_disable()
*/
void timer_ms_enable(void);

/** \brief   Disable the millisecond timer.

    This function disables the timer used for the gettime functions. Generally,
    you will not want to do this, unless you have some need to use the timer
    \ref TMU2 for something else.

    \sa timer_ms_enable()
*/
void timer_ms_disable(void);

/** \brief   Get the current uptime of the system (in secs and millisecs).

    This function retrieves the number of seconds and milliseconds since KOS was
    started.

    \note
    To get the total number of milliseconds since boot, calculate
    (*secs * 1000) + *msecs, or use the timer_ms_gettime64() function.

    \param  secs            A pointer to store the number of seconds since
                            boot.
    \param  msecs           A pointer to store the number of milliseconds past
                            a second since boot.

    \sa timer_ms_gettime64()
  
*/
void timer_ms_gettime(uint32_t *secs, uint32_t *msecs);

/** \brief   Get the current uptime of the system (in milliseconds).

    This function retrieves the number of milliseconds since KOS was started. It
    is equivalent to calling timer_ms_gettime() and combining the number of
    seconds and milliseconds into one 64-bit value.

    \return                 The number of milliseconds since KOS started.

    \sa timer_ms_gettime()
*/
uint64_t timer_ms_gettime64(void);

/** \brief   Get the current uptime of the system (in secs and microsecs).

    This function retrieves the number of seconds and microseconds since KOS was
    started.

    \note
    To get the total number of microseconds since boot, calculate
    (*secs * 1000000) + *usecs, or use the timer_us_gettime64() function.

    \param  secs            A pointer to store the number of seconds since
                            boot.
    \param  usecs           A pointer to store the number of microseconds past
                            a second since boot.

    \sa timer_us_gettime64()
*/
void timer_us_gettime(uint32_t *secs, uint32_t *usecs);

/** \brief   Get the current uptime of the system (in microseconds).

    This function retrieves the number of microseconds since KOS was started.
    It is equivalent to calling timer_us_gettime() and combining the number of
    seconds and microseconds into one 64-bit value.

    \return                 The number of microseconds since KOS started.

    \sa timer_us_gettime()
*/
uint64_t timer_us_gettime64(void);

/** \brief   Get the current uptime of the system (in secs and nanosecs).

    This function retrieves the number of seconds and nanoseconds since KOS was
    started.

    \note
    To get the total number of nanoseconds since boot, calculate
    (*secs * 1000000000) + *nsecs, or use the timer_ns_gettime64() function.

    \param  secs            A pointer to store the number of seconds since
                            boot.
    \param  nsecs           A pointer to store the number of nanoseconds past
                            a second since boot.

    \sa timer_ns_gettime64()
*/
void timer_ns_gettime(uint32_t *secs, uint32_t *nsecs);

/** \brief   Get the current uptime of the system (in nanoseconds).

    This function retrieves the number of nanoseconds since KOS was started.
    It is equivalent to calling timer_ns_gettime() and combining the number of
    seconds and nanoseconds into one 64-bit value.

    \return                 The number of nanoseconds since KOS started.

    \sa timer_ns_gettime()
*/
uint64_t timer_ns_gettime64(void);

/** @} */

/** \defgroup tmu_spin_wait Spin Waiting
    \brief                  Low-level spin waiting

    This API provides the low-level functionality used to implement thread
    spin waiting/sleeping, used by the KOS, C, C++, and POSIX threading APIs.

    @{
*/

/** \brief      Spin-loop millisecond sleep function.
    \deprecated Use timer_spin_sleep_ms().

    Compatibility macro for legacy millisecond spin sleep API.

    \param ms 	The number of milliseconds to sleep.
*/
#define timer_spin_sleep timer_spin_sleep_ms

/** \brief  Spin-loop millisecond sleep function.

    This function is meant as a very accurate delay function, even if threading
    and interrupts are disabled.

    \param  ms              The number of milliseconds to sleep.

    \sa timer_spin_sleep_us(), timer_spin_sleep_ns()
*/
void timer_spin_sleep_ms(uint32_t ms);

/** \brief 	Spin-loop microsecond sleep function.

    This function is meant as a very accurate delay function, even if threading
    and interrupts are disabled.

    \param us 	            The number of microseconds to sleep.

    \sa timer_spin_sleep_ms(), timer_spin_sleep_ns()
*/
void timer_spin_sleep_us(uint32_t us);

/** \brief      Spin-loop nanosecond sleep function.

    This function is meant as a very accurate delay function, even if threading
    and interrupts are disabled.

    \param ns               The number of nanoseconds to sleep.

    \sa timer_spin_sleep_ms(), timer_spin_sleep_us()
*/
void timer_spin_sleep_ns(uint32_t ns);

/** @} */

/** \defgroup tmu_primary   Primary Timer
    \brief                  Primary timer used by the kernel.

    This API provides a callback notification mechanism that can be hooked into
    the primary timer (\ref TMU0). It is used by the KOS kernel for threading
    and scheduling.

    \warning
    This API and its underlying functionality are using \ref TMU0, so any
    direct manipulation of it will interfere with the API's proper functioning.

    @{
*/

/** \brief   Primary timer callback type.

    This is the type of function which may be passed to
    timer_primary_set_callback() as the function that gets invoked
    upon interrupt.

    \param ctx 		The interrupt context containing saved registers.

    \sa timer_primary_set_callback()
*/
typedef void (*timer_primary_callback_t)(irq_context_t *ctx);

/** \brief   Set the primary timer callback.

    This function sets the primary timer callback to the specified function
    pointer.

    \warning
    Generally, you should not do this, as the threading system relies
    on the primary timer to work.

    \param  callback        The new timer callback (set to NULL to disable).
    \return                 The old timer callback.
*/
timer_primary_callback_t
timer_primary_set_callback(timer_primary_callback_t callback);

/** \brief   Request a primary timer wakeup.

    This function will wake the caller (by calling the primary timer callback)
    in approximately the number of milliseconds specified. You can only have one
    timer wakeup scheduled at a time. Any subsequently scheduled wakeups will
    replace any existing one.

    \param  millis          The number of milliseconds to schedule for.
*/
void timer_primary_wakeup(uint32_t millis);

/** @} */

/** \cond */
/* Init function */
int timer_init(void);
/* Shutdown */
void timer_shutdown(void);
/** \endcond */

/** @} */

__END_DECLS

#endif  /* __ARCH_TIMER_H */

