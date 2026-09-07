/* KallistiOS ##version##

   posix_timer.c
   Copyright (C) 2026 Falco Girgis
*/

/* Stubbed implementations of the asynchronous half of the POSIX Timers
   option (timer_create() and friends). */

#include <time.h>
#include <signal.h>
#include <errno.h>

int timer_create(clockid_t clock_id, struct sigevent *__restrict evp,
                 timer_t *__restrict timerid) {
    (void)clock_id;
    (void)evp;
    (void)timerid;

    errno = ENOSYS;
    return -1;
}

int timer_delete(timer_t timerid) {
    (void)timerid;

    errno = EINVAL;
    return -1;
}

int timer_settime(timer_t timerid, int flags,
                  const struct itimerspec *__restrict value,
                  struct itimerspec *__restrict ovalue) {
    (void)timerid;
    (void)flags;
    (void)value;
    (void)ovalue;

    errno = EINVAL;
    return -1;
}

int timer_gettime(timer_t timerid, struct itimerspec *value) {
    (void)timerid;
    (void)value;

    errno = EINVAL;
    return -1;
}

int timer_getoverrun(timer_t timerid) {
    (void)timerid;

    errno = EINVAL;
    return -1;
}
