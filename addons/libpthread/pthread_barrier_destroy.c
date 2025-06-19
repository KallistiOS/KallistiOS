/* KallistiOS ##version##

   pthread_barrier_destroy.c
   Copyright (C) 2024 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <kos/barrier.h>

int pthread_barrier_destroy(pthread_barrier_t *barrier) {
    return thd_barrier_destroy(barrier);
}
