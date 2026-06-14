/* KallistiOS ##version##

   posix_timer.c
   Copyright (C) 2026 Falco Girgis
*/

#include <time.h>
#include <errno.h>

int timer_create(clockid_t clock_id, struct sigevent *evp, timer_t *timerid) {
    (void)clock_id;
    (void)evp;
    (void)timerid;

    return ENOTSUP;
}

int timer_delete(timer_t timerid) {
    (void)timerid;

    return EINVAL;
}

int timer_settime(timer_t timerid, int flags, const struct itimerspec *value,
                  struct itimerspec *ovalue) {
    (void)timerid;
    (void)flags;
    (void)value;
    (void)ovalue;
    
    return EINVAL;
}

int timer_gettime(timer_t timerid, struct itimerspec *value) {
    (void)timerid;
    (void)value;

    return EINVAL;
}

int timer_getoverrun(timer_t timerid) {
    (void)timerid;

    return EINVAL;
}