/* KallistiOS ##version##

   arch/xbox/include/arch/rtc.h
   Copyright (C) 2026 The KallistiOS Contributors
*/

/** \file    arch/rtc.h
    \brief   Xbox real-time clock fallback.

    The Xbox port does not yet own an RTC device. This interface deliberately
    exposes that limitation instead of depending on an Xbox-kernel service.
    Until a native driver exists, wall-clock time has an epoch of system boot
    and attempts to set it fail with ENOSYS.
*/

#ifndef __ARCH_XBOX_RTC_H
#define __ARCH_XBOX_RTC_H

#include <arch/timer.h>

#include <errno.h>
#include <time.h>

static inline time_t arch_rtc_unix_secs(void) {
    return arch_timer_gettime().tv_sec;
}

static inline int arch_rtc_set_unix_secs(time_t value) {
    (void)value;
    errno = ENOSYS;
    return -1;
}

static inline time_t arch_rtc_boot_time(void) {
    return 0;
}

static inline int arch_rtc_init(void) {
    return 0;
}

static inline void arch_rtc_shutdown(void) {
}

#endif /* __ARCH_XBOX_RTC_H */
