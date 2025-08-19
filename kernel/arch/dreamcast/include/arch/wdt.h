/* KallistiOS ##version##

   arch/dreamcast/include/wdt.h
   Copyright (C) 2023 Falco Girgis

*/

/** \file    arch/wdt.h
    \brief   Watchdog Timer API
    \ingroup wdt

    This file provides an API for configuring and utilizing the SH4's watchdog
    timer as either a reset or an interval timer.

    \sa arch/timer.h
    \sa arch/rtc.h

    \author Falco Girgis
*/

#ifndef __ARCH_WDT_H
#define __ARCH_WDT_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

/** \defgroup wdt   Watchdog Timer
    \brief          Driver for using the WDT as a reset or interval timer
    \ingroup        timing

    The watchdog timer (WDT) is a special-purpose timer peripheral integrated
    within the Dreamcast's SH4 CPU.

    \warning
    At this time, there are no known emulators which are emulating the WDT,
    as it was never used in commercial games; however, it works perfectly fine
    on real hardware.

    There are two different modes of operation which are supported:
        - watchdog mode: counter overflow causes a reset interrupt
        - interval timer mode: counter overflow invokes a callback function

    To start the WDT in watchdog mode, use wdt_enable_watchdog(). To use the
    WDT as a general-purpose interval timer, use wdt_enable_timer().

    The timer can be stopped in either mode by calling wdt_disable().

    \sa rtc
*/

/** \brief   Clock divider settings
    \ingroup wdt

    Denominators used to set the frequency divider
    for the input clock to the WDT.
 */
typedef enum WDT_CLK_DIV {
    WDT_CLK_DIV_32,     /**< \brief Period: 41us */
    WDT_CLK_DIV_64,     /**< \brief Period: 82us */
    WDT_CLK_DIV_128,    /**< \brief Period: 164us */
    WDT_CLK_DIV_256,    /**< \brief Period: 328us */
    WDT_CLK_DIV_512,    /**< \brief Period: 656us */
    WDT_CLK_DIV_1024,   /**< \brief Period: 1.31ms */
    WDT_CLK_DIV_2048,   /**< \brief Period: 2.62ms */
    WDT_CLK_DIV_4096    /**< \brief Period: 5.25ms */
} WDT_CLK_DIV;

/** \brief   Reset signal type
    \ingroup wdt

    Specifies the kind of reset to be performed when the WDT
    overflows in watchdog mode.
*/
typedef enum WDT_RST {
    WDT_RST_POWER_ON,   /**< \brief Power-On Reset */
    WDT_RST_MANUAL      /**< \brief Manual Reset */
} WDT_RST;

/* \brief   WDT interval timer callback function type
   \ingroup wdt

   Type of the callback function to be passed to wdt_enable_timer().
*/
typedef void (*wdt_callback)(void *user_data);

/** \brief   Enables the WDT as an interval timer
    \ingroup wdt

    Stops the WDT if it was previously running and reconfigures it
    to be used as a generic interval timer, calling the given callback
    periodically at the requested interval (or as close to it as possible
    without calling it prematurely).

    \note
    The internal resolution for each tick of the WDT in this mode is
    41us, meaning a requested \p microsec_period of 100us will result
    in an actual callback interval of 123us.

    \warning
    \p callback is invoked within an interrupt context, meaning that
    special care should be taken to not perform any logic requiring
    additional interrupts. Data that is accessed from both within
    and outside of the callback should be atomic or protected by a 
    lock.

    \param  initial_count   Initial value of the WDT counter (Normally 0).
    \param  microsec_period Timer callback interval in microseconds
    \param  irq_prio        Priority for the interval timer IRQ (1-15)
    \param  callback        User function to invoke periodically
    \param  user_data       Arbitrary user-provided data for the callback

    \sa wdt_disable()
*/
void wdt_enable_timer(uint8_t initial_count,
                      uint32_t microsec_period,
                      uint8_t irq_prio,
                      wdt_callback callback,
                      void *user_data);

/** \brief   Enables the WDT in watchdog mode
    \ingroup wdt

    Stops the WDT if it was previously running and reconfigures it
    to be used as a typical watchdog timer, generating a reset
    interrupt upon counter overflow. To prevent this from happening,
    the user should be periodically resetting the counter.

    \note
    Keep in mind the speed of the WDT. With a range of 41us to 5.2ms,
    the WDT will overflow before a single frame in a typical game.

    \param  initial_count   Initial value of the WDT counter (Normally 0)
    \param  clk_config      Clock divider to set watchdog period
    \param  reset_select    The type of reset generated upon overflow

    \sa wdt_disable()
*/
void wdt_enable_watchdog(uint8_t initial_count,
                         WDT_CLK_DIV clk_config,
                         WDT_RST reset_select);

/** \brief   Fetches the counter value
    \ingroup wdt

    Returns the current 8-bit value of the WDT counter.

    \return     Current counter value

    \sa wdt_set_counter()
*/
uint8_t wdt_get_counter(void);

/** \brief   Sets the counter value
    \ingroup wdt

    Sets the current 8-bit value of the WDT counter.

    \param  value       New value for the counter

    \sa wdt_get_counter(), wdt_pet()
*/
void wdt_set_counter(uint8_t value);

/** \brief   Resets the counter value
    \ingroup wdt

    "Petting" or "kicking" the WDT is the same thing as
    resetting its counter value to 0.

    \sa wdt_set_counter()
*/
void wdt_pet(void);

/** \brief   Disables the WDT
    \ingroup wdt

    Disables the WDT if it was previously enabled,
    otherwise does nothing.

    \sa wdt_enable_timer(), wdt_enable_watchdog()
*/
void wdt_disable(void);

/** \brief   Returns whether the WDT is enabled
    \ingroup wdt

    Checks to see whether the WDT has been enabled.

    \return     1 if enabled, 0 if disabled
*/
int wdt_is_enabled(void);

__END_DECLS

#endif  /* __ARCH_WDT_H */
