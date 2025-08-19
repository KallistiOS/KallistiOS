/* KallistiOS ##version##

   clock_gettime.c
   Copyright (C) 2023, 2024 Falco Girgis
*/

#include <kos/thread.h>

#include <arch/timer.h>
#include <arch/rtc.h>

#include <time.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>

int clock_getcpuclockid(pid_t pid, clockid_t *clock_id) {
    /* pid of 0 means the current process,
       and we only support a single process. */
    if(pid != 0 || pid != KOS_PID)
        return ESRCH;

    assert(clock_id);

    *clock_id = CLOCK_PROCESS_CPUTIME_ID;

    return 0;
}

int clock_getres(clockid_t clk_id, struct timespec *ts) {
    switch(clk_id) {
        case CLOCK_REALTIME:
        case CLOCK_MONOTONIC:
        case CLOCK_PROCESS_CPUTIME_ID:
        case CLOCK_THREAD_CPUTIME_ID:
            if(!ts) {
                errno = EFAULT;
                return -1;
            }

            ts->tv_sec = 0;
            ts->tv_nsec = 1;
            return 0;
            
        default:
            errno = EINVAL;
            return -1;
    }
}

int clock_gettime(clockid_t clk_id, struct timespec *ts) {
    lldiv_t  div_result;
    uint64_t ns64;
    uint32_t secs, nsecs, secs_offset=0;

    if(!ts) {
        errno = EFAULT;
        return -1;
    }

    switch(clk_id) {
        /* Use the nanosecond resolution boot time
           + RTC bios time */
        case CLOCK_REALTIME:
            secs_offset = rtc_boot_time();
            /* fall through */

        /* Use the nanosecond resolution boot time */
        case CLOCK_MONOTONIC:
            timer_ns_gettime(&secs, &nsecs);
            ts->tv_sec = secs + secs_offset;
            ts->tv_nsec = nsecs;
            return 0;

        /* Use the sum of all kthread-specific CPU time counters */
        case CLOCK_PROCESS_CPUTIME_ID:
            div_result = lldiv(thd_get_total_cpu_time(), 1000000000);
            ts->tv_sec = div_result.quot;
            ts->tv_nsec = div_result.rem;
            return 0;

        /* Use the kthread-specific CPU time counters */
        case CLOCK_THREAD_CPUTIME_ID:
            ns64 = thd_get_cpu_time(thd_get_current());
            div_result = lldiv(ns64, 1000000000);
            ts->tv_sec = div_result.quot;
            ts->tv_nsec = div_result.rem;
            return 0;

        default:
            errno = EINVAL;
            return -1;
    }
}

int clock_settime(clockid_t clk_id, const struct timespec *ts) {
    switch(clk_id) {
        case CLOCK_REALTIME:
            if(!ts) {
                errno = EFAULT;
                return -1;
            }

            return rtc_set_unix_secs(ts->tv_sec);

        default:
            errno = EINVAL;
            return -1;
    }
}
