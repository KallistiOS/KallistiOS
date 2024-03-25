/* KallistiOS ##version##

   kos/time.h
   Copyright (C) 2023 Lawrence Sebald
   Copyright (C) 2024 Falco Girgis
*/

/** \file    kos/time.h
    \brief   KOS-implementation of select C11 and POSIX extensions

    Add select POSIX extensions and C11 functionality to time.h which are not
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

#if !defined(__STRICT_ANSI__) || (__STDC_VERSION__ >= 201112L) || (__cplusplus >= 201703L)

/* Forward declaration. */
struct timespec;

#define TIME_UTC 1

/* Microsecond resolution for clock(), per POSIX standard */
#define CLOCKS_PER_SEC 1000000

/* C11 nanosecond-resolution timing. */
extern int timespec_get(struct timespec *ts, int base);

#endif /* !defined(__STRICT_ANSI__) || (__STDC_VERSION__ >= 201112L) || (__cplusplus >= 201703L) */

#if !defined(__STRICT_ANSI__) || (_POSIX_C_SOURCE >= 199309L)

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

/* We do NOT support thread-specific CPU time!
#ifndef _POSIX_THREAD_CPU_TIME
#define _POSIX_THREAD_CPUTIME 1
#endif
*/

/* Explicitly provided function declarations for POSIX clock API, since
   getting them from Newlib requires supporting the rest of the _POSIX_TIMERS
   API, which is not implemented yet. */
extern int clock_settime(__clockid_t clock_id, const struct timespec *ts);
extern int clock_gettime(__clockid_t clock_id, struct timespec *ts);
extern int clock_getres(__clockid_t clock_id, struct timespec *res);

#endif /* !defined(__STRICT_ANSI__) || (_POSIX_C_SOURCE >= 199309L) */

__END_DECLS

#endif /* !__KOS_TIME_H */
