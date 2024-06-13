/* KallistiOS ##version##

   pthread_barrier_init.c
   Copyright (C) 2024 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <kos/barrier.h>

int pthread_barrier_init(pthread_barrier_t *__RESTRICT barrier,
                         const pthread_barrierattr_t *__RESTRICT attr,
                         unsigned count) {
    return thd_barrier_init(barrier, attr, count);
}
