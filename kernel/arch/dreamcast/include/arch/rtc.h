/* KallistiOS ##version##

   arch/dreamcast/include/rtc.h
   (c)2000-2001 Megan Potter

*/

/** \file   arch/rtc.h
    \brief  Low-level real time clock functionality.

    This file contains functions for interacting with the real time clock in the
    Dreamcast. Generally, you should prefer interacting with the higher level
    standard C functions, like time(), rather than these.

    \author Megan Potter
*/

#ifndef __ARCH_RTC_H
#define __ARCH_RTC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <time.h>

/** \brief  Get the current date/time.

    This function retrieves the current RTC value as a standard UNIX timestamp
    (with an epoch of January 1, 1970 00:00). This is assumed to be in the
    timezone of the user (as the RTC does not support timezones).

    \return                 The current UNIX-style timestamp (local time).
*/
time_t rtc_unix_secs();

/** \brief  Get the time that the sytem was booted.

    This function retrieves the RTC value from when KallistiOS was started. As
    with rtc_unix_secs(), this is a UNIX-style timestamp in local time.

    \return                 The boot time as a UNIX-style timestamp.
*/
time_t rtc_boot_time();

/* \cond */
/* Init / Shutdown */
int rtc_init();
void rtc_shutdown();
/* \endcond */

__END_DECLS

#endif  /* __ARCH_RTC_H */

