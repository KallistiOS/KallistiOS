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

__END_DECLS

#endif /* !defined(__STRICT_ANSI__) || (__STDC_VERSION__ >= 201112L) || (__cplusplus >= 201703L) */
#endif /* !__KOS_TIME_H */
