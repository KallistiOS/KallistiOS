/* KallistiOS ##version##

   newlib_gettimeofday.c
   Copyright (C)2002,2004 Megan Potter

*/

#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <arch/timer.h>
#include <arch/rtc.h>

/* This is kind of approximate and works only with "localtime" */
int _gettimeofday_r(void * re, struct timeval *tv, struct timezone *tz) {
    uint32  m, s;

    (void)re;
    (void)tz;

    assert(tv != NULL);

    timer_ms_gettime(&s, &m);
    tv->tv_sec = rtc_boot_time() + s;
    tv->tv_usec = m * 1000;

    return 0;
}
