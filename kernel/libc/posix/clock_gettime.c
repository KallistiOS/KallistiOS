/* KallistiOS ##version##

   clock_gettime.c
   Copyright (C) 2023 Falco Girgis
*/

#include <time.h>
#include <arch/timer.h>
#include <arch/rtc.h>
#include <errno.h>

int clock_getres(clockid_t clk_id, struct timespec *res) {
    switch(clk_id) {
    case CLOCK_REALTIME:
    case CLOCK_MONOTONIC:
    case CLOCK_PROCESS_CPUTIME_ID:
    case CLOCK_THREAD_CPUTIME_ID:
        if(!res) {
            errno = EFAULT;
            return -1;
        }
        res->tv_sec = 0;
        res->tv_nsec = 1000 * 1000;
        return 0;
    default:
        errno = EINVAL;
        return -1;
    }
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    if(!tp) {
        errno = EFAULT;
        return -1;
    }

    switch(clk_id) {
    case CLOCK_REALTIME:
        return timespec_get(tp, TIME_UTC) == TIME_UTC? 0 : -1;
    case CLOCK_MONOTONIC:
    case CLOCK_PROCESS_CPUTIME_ID:
    case CLOCK_THREAD_CPUTIME_ID: {
        uint32_t secs, msecs;
        timer_ms_gettime(&secs, &msecs);
        tp->tv_sec = secs;
        tp->tv_nsec = msecs * 1000 * 1000;
        return 0;
    }
    default:
        errno = EINVAL;
        return -1;
    }
}

int clock_settime(clockid_t clk_id, const struct timespec *tp) {
    switch(clk_id) {
    case CLOCK_REALTIME:
        if(!tp) {
            errno = EFAULT;
            return -1;
        }
        return rtc_set_unix_secs(tp->tv_sec); 
    default:
        errno = EINVAL;
        return -1;
    }
}
