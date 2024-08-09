/* KallistiOS ##version##

   kos/time.h
   Copyright (C) 2023 Lawrence Sebald
   Copyright (C) 2024 Falco Girgis
*/

/** \file    kos/time.h
    \brief   KOS-implementation of select C11 and POSIX extensions

    Add select POSIX extensions, C11, and C23 functionality to time.h which are not
    present within Newlib.

    \remark
    This will probably go away at some point in the future, if/when Newlib gets
    an implementation of this function. But for now, it's here.

    \todo
    - Implement _POSIX_TIMERS, which requires POSIX signals back-end.
    - Implement thread-specific CPU time

    \author Lawrence Sebald
    \author Falco Girgis
*/

#ifndef _TIME_H_
   #error "Do not include this file directly. Use <time.h> instead."
#endif /* !_TIME_H_ */

#ifndef __KOS_TIME_H
#define __KOS_TIME_H

#include <kos/cdefs.h>

__BEGIN_DECLS

/** \cond */

/* Required definition for a fully C23-compliant <time.h> */
#define __STDC_VERSION_TIME_H__  202311L

/* Microsecond resolution for clock(), per POSIX standard.. */
#define CLOCKS_PER_SEC           1000000

/* =============== Enable the following for >=c11, >=c++17 =================== */
#if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)) || \
    (defined(__cplusplus)      && (__cplusplus >= 201703L))

/* Only supported base time in C11. */
#define TIME_UTC 1

/* C11 nanosecond-resolution timing. */
struct timespec;
extern int timespec_get(struct timespec *ts, int base);
#endif

/* ============ Enable the following for >=C2x, >C++20+, KOS ============== */
#if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202000L)) || \
    (defined(__cplusplus)      && (__cplusplus > 202002L))       || \
     defined(__KOS_LIBC)

/* New POSIX-equivalent base times in C23. */
#define TIME_MONOTONIC     2
#define TIME_ACTIVE        3
#define TIME_THREAD_ACTIVE 4

/* Query for the resolution of a time base. */
extern int timespec_getres(struct timespec *ts, int base);
#endif

/* ===================== Enable the following for >=c2y ================== */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202300L)

/* Deprecate legacy time formatters with static internal state. */
struct tm;
[[deprecated]] extern char *asctime(const struct tm *timeptr);
[[deprecated]] extern char *ctime(const __time_t *timer);
#endif

/* =========== Enable the following for >=c2y, >c++20, KOS ============== */
#if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202300L)) || \
    (defined(__cplusplus)      && (__cplusplus > 202002L))       || \
     defined(__KOS_LIBC)

/* POSIX reentrant equivalents of gmtime and localtime added to C23. */
struct tm;
extern struct tm *gmtime_r(const __time_t *timer, struct tm *timeptr);
extern struct tm *localtime_r(const __time_t *timer, struct tm *timeptr);

/* C23 added POSIX gmtime() for UTC broken-down time to a Unix timestamp. */
extern __time_t timegm(struct tm *timeptr);

#endif

/* =========== Enable the following for POSIX POSIX.1b (1993) =========== */
#if defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 199309L)

/* We do not support POSIX timers!
#ifndef _POSIX_TIMERS
#define _POSIX_TIMERS 1
#endif */

#ifndef _POSIX_MONOTONIC_CLOCK
#define _POSIX_MONOTONIC_CLOCK 1
#endif

#ifndef _POSIX_CPUTIME
#define _POSIX_CPUTIME 1
#endif

#ifndef _POSIX_THREAD_CPUTIME
#define _POSIX_THREAD_CPUTIME 1
#endif

/* Explicitly provided function declarations for POSIX clock API, since
   getting them from Newlib requires supporting the rest of the _POSIX_TIMERS
   API, which is not implemented yet. */
extern int clock_settime(__clockid_t clock_id, const struct timespec *ts);
extern int clock_gettime(__clockid_t clock_id, struct timespec *ts);
extern int clock_getres(__clockid_t clock_id, struct timespec *res);

extern int nanosleep(const struct timespec *req, struct timespec *rem);

#endif

/** \endcond */

__END_DECLS

#endif /* !__KOS_TIME_H */
