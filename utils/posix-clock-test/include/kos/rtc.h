#ifndef TEST_KOS_RTC_H
#define TEST_KOS_RTC_H
#include <time.h>
time_t rtc_boot_time(void);
int rtc_set_unix_secs(time_t seconds);
#endif
