#ifndef __ARCH_RTC_H
#define __ARCH_RTC_H

#include <time.h>

time_t rtc_unix_secs();
time_t rtc_boot_time();

#endif  /* __ARCH_RTC_H */