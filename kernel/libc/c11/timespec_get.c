/* KallistiOS ##version##

   timespec_get.c
   Copyright (C) 2023 Lawrence Sebald
   Copyright (C) 2023, 2024 Falco Girgis
*/

#include <time.h>
#include <errno.h>
#include <stdbool.h>

/* Convert a C11 time base into a POSIX Clock ID. */
static inline bool posix_clk_id(int base, clockid_t *id) {
    switch(base) {
        case TIME_UTC:
            *id = CLOCK_REALTIME;
            return true;

        case TIME_MONOTONIC:
            *id = CLOCK_MONOTONIC;
            return true;

        case TIME_ACTIVE:
            *id = CLOCK_PROCESS_CPUTIME_ID;
            return true;

        case TIME_THREAD_ACTIVE:
            *id = CLOCK_THREAD_CPUTIME_ID;
            return true;

        default:
            return false;
    }
}

/* Proxy a call to a C11/C23 timespec function to a POSIX clock function. */
static int timespec_posix_adapter(struct timespec *ts,
                                  int base,
                                  int (*clockfn)(clock_t, struct timespec *)) {
    clockid_t clk_id;

    if(!posix_clk_id(base, &clk_id))
        return 0;

    if(!ts)
        return 0;

    /* errno is not modified from the C11/C23 timespec API! */
    const int old_errno = errno;
    if(clockfn(clk_id, ts) == -1) {
        errno = old_errno;
        return 0;
    }

    return base;
}

/* C11 timespec_get() -> POSIX clock_gettime(). */
int timespec_get(struct timespec *ts, int base) {
    return timespec_posix_adapter(ts, base, clock_gettime);
}

/* C23 timespec_getres() -> POSIX clock_getres(). */
int timespec_getres(struct timespec *ts, int base) {
    return timespec_posix_adapter(ts, base, clock_getres);
}
