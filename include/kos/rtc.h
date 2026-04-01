/* KallistiOS ##version##

   include/kos/rtc.h
   Copyright (C) 2000, 2001 Megan Potter
   Copyright (C) 2023, 2024 Falco Girgis

*/

/** \file    kos/rtc.h
    \brief   Low-level real-time clock functionality.
    \ingroup rtc

    This file contains functions for interacting with the real-time clock.
    Generally, you should prefer interacting with the higher level standard C
    functions, like time(), rather than these when simply needing to fetch the
    current system time.

    \author Megan Potter
    \author Falco Girgis
*/

#ifndef __KOS_RTC_H
#define __KOS_RTC_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <arch/rtc.h>

#include <time.h>

/** \defgroup rtc Real-Time Clock
    \brief        Real-Time Clock (RTC) Management
    \ingroup      timing

    Provides an API for fetching and managing the date/time using
    the hardware real-time clock (RTC). All timestamps are in standard
    Unix format, with an epoch of January 1, 1970. Due to the fact
    that there is no time zone data on the RTC, all times are expected
    to be in the local time zone.

    \note
    For reading the current date/time, you should favor the standard C,
    C++, or POSIX functions, as they are platform-indpendent and are
    calculating current time based on a cached boot time plus a delta
    that is maintained by the timer subsystem, rather than actually
    having to requery the RTC, so they are faster.

    \sa wdt, timers, perf_counters

    @{
*/

/** \brief   Get the current date/time.

    This function retrieves the current RTC value as a standard UNIX timestamp
    (with an epoch of January 1, 1970 00:00). This is assumed to be in the
    timezone of the user (as the RTC does not support timezones).

    \return                 The current UNIX-style timestamp (local time).

    \sa rtc_set_unix_secs()
*/
static inline time_t rtc_unix_secs(void) {
    return arch_rtc_unix_secs();
}

/** \brief   Set the current date/time.

    This function sets the current RTC value as a standard UNIX timestamp
    (with an epoch of January 1, 1970 00:00). This is assumed to be in the
    timezone of the user (as the RTC does not support timezones).

    \warning
    This function may fail! Since `time_t` is typically 64-bit while the RTC
    uses a 32-bit timestamp (which also has a different epoch), not all
    `time_t` values can be represented within the RTC!

    \param      time        Unix timestamp to set the current time to

    \return                 0 for success or -1 for failure (with errno set
                            appropriately).

    \exception  EINVAL      \p time was an invalid timestamp or could not be
                            represented on the hardware RTC.
    \exception  EPERM       Failed to set and successfully read back \p time
                            from the RTC.

    \sa rtc_unix_secs()
*/
static inline int rtc_set_unix_secs(time_t time) {
    return arch_rtc_set_unix_secs(time);
}

/** \brief   Get the time since the system was booted.

    This function retrieves the cached RTC value from when KallistiOS was
    started. As with rtc_unix_secs(), this is a UNIX-style timestamp in
    local time.

    \return                 The boot time as a UNIX-style timestamp.
*/
static inline time_t rtc_boot_time(void) {
    return arch_rtc_boot_time();
}

/* \cond INTERNAL */
/* Internally called Init / Shutdown */
static inline int rtc_init(void) {
    return arch_rtc_init();
}

static inline void rtc_shutdown(void) {
    arch_rtc_shutdown();
}
/* \endcond */

/** @} */

__END_DECLS

#endif  /* __KOS_RTC_H */
