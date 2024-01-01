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
#define CLOCKS_PER_SEC 1000000

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

#endif /* !defined(__STRICT_ANSI__) || (_POSIX_C_SOURCE >= 199309L) */

__END_DECLS

#endif /* !__KOS_TIME_H */
