/* KallistiOS ##version##

   kos/time.h
   Copyright (C) 2023 Lawrence Sebald
*/

/* This will probably go away at some point in the future, if/when Newlib gets
   an implementation of this function. But for now, it's here. */

#ifndef _TIME_H_
#error "Do not include this file directly. Use <time.h> instead."
#endif /* !_TIME_H_ */

#ifndef __KOS_TIME_H
#define __KOS_TIME_H

#if !defined(__STRICT_ANSI__) || (__STDC_VERSION__ >= 201112L) || (__cplusplus >= 201703L)

#include <kos/cdefs.h>

__BEGIN_DECLS

/* Forward declaration. */
struct timespec;

#define TIME_UTC 1

extern int timespec_get(struct timespec *ts, int base);

#ifndef __STRICT_ANSI__

#ifndef _POSIX_TIMERS
#define _POSIX_TIMERS 1
#endif 
#ifdef _POSIX_MONOTONIC_CLOCK
#define _POSIX_MONOTONIC_CLOCK 1
#endif
#ifdef _POSIX_CPUTIME
#define _POSIX_CPUTIME 1
#endif
#ifdef _POSIX_THREAD_CPU_TIME
#define _POSIX_THREAD_CPUTIME 1
#endif

#define CLOCK_MONOTONIC          (CLOCK_REALTIME + 1)
#define CLOCK_PROCESS_CPUTIME_ID (CLOCK_REALTIME + 2)
#define CLOCK_THREAD_CPUTIME_ID  (CLOCK_REALTIME + 3)

extern int clock_getres(__clockid_t clk_id, struct timespec *res);
extern int clock_gettime(__clockid_t clk_id, struct timespec *tp);
extern int clock_settime(__clockid_t clk_id, const struct timespec *tp);

#endif /* !defined(__STRICT_ANSI__) */

__END_DECLS

#endif /* !defined(__STRICT_ANSI__) || (__STDC_VERSION__ >= 201112L) || (__cplusplus >= 201703L) */
#endif /* !__KOS_TIME_H */
