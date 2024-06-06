/* KallistiOS ##version##

   timegm.c
   Copyright (C) 2024 Falco Girgis
*/

#include <time.h>
#include <errno.h>
#include <stdbool.h>

/*  C23/POSIX timegm() converts a UTC-based broken-down time to a unix
    timestamp... and since we have no timezone on the Dreamcast (yet),
    our local time is already UTC, which means this is going to be
    equivalent to using mktime()! 
*/
time_t timegm(struct tm *timeptr) {
    return mktime(timeptr);
}

