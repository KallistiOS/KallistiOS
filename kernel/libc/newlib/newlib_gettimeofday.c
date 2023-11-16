/* KallistiOS ##version##

   newlib_gettimeofday.c
   Copyright (C) 2002, 2004 Megan Potter
   Copyright (C) 2023 Falco Girgis
*/

#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <arch/timer.h>
#include <arch/rtc.h>

/* This is kind of approximate and works only with "localtime" */
int _gettimeofday_r(void * re, struct timeval *tv, struct timezone *tz) {
    uint32_t u, s;

    (void)re;
    (void)tz;

    assert(tv != NULL);

    timer_us_gettime(&s, &u);
    tv->tv_sec = rtc_boot_time() + s;
    tv->tv_usec = u;

    return 0;
}
