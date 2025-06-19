/* KallistiOS ##version##

   pthread_mutex_consistent.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_mutex_consistent(pthread_mutex_t *mutex) {
    if(!mutex)
        return EINVAL;

    /* We do not currently support robust mutexes, so always return EINVAL to
       indicate that the mutex is not robust. */
    return EINVAL;
}
