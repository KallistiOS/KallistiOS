/* KallistiOS ##version##

   arch/dreamcast/include/arch/rtc.h
   Copyright (C) 2000, 2001 Megan Potter
   Copyright (C) 2023, 2024 Falco Girgis
   Copyright (C) 2026 Paul Cercueil

*/

/** \file    arch/rtc.h
    \brief   Low-level real-time clock functionality.
    \ingroup rtc

    This file contains functions for interacting with the real-time clock in the
    Dreamcast. Generally, you should prefer interacting with the higher level
    standard C functions, like time(), rather than these when simply needing
    to fetch the current system time.

    \sa dc/wdt.h

    \author Megan Potter
    \author Falco Girgis
*/

/* Keep this include above the macro guards */
#include <kos/rtc.h>

#ifndef __ARCH_RTC_H
#define __ARCH_RTC_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <time.h>

/* Notes:
    The RTC that is used by the DC is located on the AICA rather than SH4,
    presumably for power-efficiency reasons. Because of this, accessing
    it requires a trip over the G2 BUS, which is notoriously slow.

    Internally, the RTC's date/time is maintained using a 32-bit counter
    with an epoch of January 1, 1950 00:00. Because of this, the Dreamcast's
    Y2K and the last timestamp it can represent before rolling over is
    February 06 2086 06:28:15.
*/

extern time_t dc_boot_time;

time_t arch_rtc_unix_secs(void);
int arch_rtc_set_unix_secs(time_t time);

static inline time_t arch_rtc_boot_time(void) {
    return dc_boot_time;
}

static inline int arch_rtc_init(void) {
    dc_boot_time = arch_rtc_unix_secs();
    return 0;
}

static inline void arch_rtc_shutdown(void) {
}

__END_DECLS

#endif  /* __ARCH_RTC_H */

