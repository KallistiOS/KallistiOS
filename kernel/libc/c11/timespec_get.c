/* KallistiOS ##version##

   timespec_get.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include <time.h>
#include <arch/timer.h>
#include <arch/rtc.h>

int timespec_get(struct timespec *ts, int base) {
    if(base == TIME_UTC && ts) {
        uint32 s, ms;

        timer_ms_gettime(&s, &ms);
        ts->tv_sec = rtc_boot_time() + s;
        ts->tv_nsec = ms * 1000 * 1000;

        return base;
    }

    return 0;
}
