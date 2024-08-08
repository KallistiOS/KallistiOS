/* KallistiOS ##version##

   arch/dreamcast/include/rtc.h
   Copyright (C) 2000, 2001 Megan Potter
   Copyright (C) 2023, 2024 Falco Girgis

*/

/** \file    arch/rtc.h
    \brief   Low-level real-time clock functionality.
    \ingroup rtc

    This file contains functions for interacting with the real-time clock in the
    Dreamcast. Generally, you should prefer interacting with the higher level
    standard C functions, like time(), rather than these when simply needing
    to fetch the current system time.

    \sa arch/wdt.h

    \author Megan Potter
    \author Falco Girgis
*/

#ifndef __ARCH_RTC_H
#define __ARCH_RTC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <time.h>

/** \defgroup rtc Real-Time Clock
    \brief        Real-Time Clock (RTC) Management
    \ingroup      timing 

    Provides an API for fetching and managing the date/time using
    the Dreamcast's real-time clock. All timestamps are in standard
    Unix format, with an epoch of January 1, 1970. Due to the fact
    that there is no time zone data on the RTC, all times are expected
    to be in the local time zone.

    \note
    The RTC that is used by the DC is located on the AICA rather than SH4, 
    presumably for power-efficiency reasons. Because of this, accessing 
    it requires a trip over the G2 BUS, which is notoriously slow.

    \note
    For reading the current date/time, you should favor the standard C,
    C++, or POSIX functions, as they are platform-indpendent and are 
    calculating current time based on a cached boot time plus a delta
    that is maintained by the timer subsystem, rather than actually
    having to requery the RTC over the G2 BUS, so they are faster.

    \warning
    Internally, the RTC's date/time is maintained using a 32-bit counter 
    with an epoch of January 1, 1950 00:00. Because of this, the Dreamcast's
    Y2K and the last timestamp it can represent before rolling over is 
    February 06 2086 06:28:15.

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
time_t rtc_unix_secs(void);

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
                            represented on the AICA's RTC.
    \exception  EPERM       Failed to set and successfully read back \p time
                            from the RTC.

    \sa rtc_unix_secs()
*/
int rtc_set_unix_secs(time_t time);

/** \brief   Get the time since the system was booted.

    This function retrieves the cached RTC value from when KallistiOS was
    started. As with rtc_unix_secs(), this is a UNIX-style timestamp in
    local time.

    \return                 The boot time as a UNIX-style timestamp.
*/
time_t rtc_boot_time(void);

/* \cond INTERNAL */
/* Internally called Init / Shutdown */
int rtc_init(void);
void rtc_shutdown(void);
/* \endcond */

/** @} */

__END_DECLS

#endif  /* __ARCH_RTC_H */

