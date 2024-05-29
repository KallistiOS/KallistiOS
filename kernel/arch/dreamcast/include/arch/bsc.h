/* KallistiOS ##version##

   arch/dreamcast/include/bsc.h
   Copyright (c) 2023 Falco Girgis

*/

/** \file   arch/bsc.h
    \brief  Bus State Controller API

    \sa timer.h
    \sa wdt.h 
    \sa rtc.h

    \author Falco Girgis
*/

#ifndef __ARCH_BSC_H
#define __ARCH_BSC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <time.h>
#include <stdint.h>
#include <stdbool.h>

/** \defgroup bsc    Bus State Controller (BSC)
    \brief           Driver for the SH4's BSC Peripheral

   \sa timers
   \sa wdt
*/   

/** \brief Timeout Handler for BSC Timer
    
    The following function type is used to provide a timeout callback
    handler to bsc_timer_prime(). Upon timeout, it will be invoked with
    a current time offset (for managing nanosecond inaccuracies) and the
    user_data pointer which was originally supplied with it.
*/
typedef void (*bsc_callback_t)(void* user_data);

/** \brief Configures the BSC Periodic Timer
   
    This function configures, but does not actually start, the BSC's memory
    refresh timer as a general-purpose nanosecond-resolution interval timer,
    which invokes \p timeout_handler when the given \p period elapses.

    \param interval          The requested time before the timeout handler is 
                             called.
    \param periodic          false if the timeout is to only be called once,
                             true if the timeout should be called repeatedly.
    \param timeout_handler   The user callback function for the driver to call
                             upon timeout.
    \param user_data         Arbitrary user-pointer to pass to the handler

    \retval true             Success
    \retval false            Failure 

    \sa bsc_timer_start()
*/
bool bsc_timer_interval(const struct timespec *interval, 
                        bool periodic,
                        bsc_callback_t timeout_handler,
                        void *user_data);

/** \brief Begins Running the BSC's Interval Timer
    
    Starts the timer by supplying its clock source.

    \sa bsc_timer_stop(), bsc_timer_running()
*/
bool bsc_timer_start(void);

/** \brief Stops the BSC's Interval Timer
    
    Stops (but does not reset) the currently running timer by removing its
    clock source. It can be resumed again later.

    \sa bsc_timer_start(), bsc_timer_running()
*/
bool bsc_timer_stop(void);

/** \brief Resets the BSC's Interval Timer

    Clears the internal elapsed time variables and timer counter registers
    back to their initial values.
*/
bool bsc_timer_reset(void);

/** \brief Returns whether the BSC"s Interval Timer is Active 

    Returns whether the timer is running or not by checking whether it has a
    clock source supplied to it.
*/
bool bsc_timer_running(void);

/** \brief Returns BSC's Interval Timer Elapsed Time 

    This function returns the total amount of time that has elapsed between
    making calls to bsc_timer_start() and bsc_timer_stop(). The total ealapsed
    time can accumulate between multiple runs without calling bsc_timer_reset().
*/
bool bsc_timer_elapsed(struct timespec *time);



void bsc_set_isrs(uint8_t priority, 
                  bsc_callback_t comp_match_callback, void *comp_match_data,
                  bsc_callback_t overflow_callback, void *overflow_data);



void bsc_init(void);

void bsc_shutdown(void);

__END_DECLS

#endif  /* __ARCH_BSC_H */
