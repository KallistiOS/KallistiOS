/* KallistiOS ##version##

   pthread_spin_destroy.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <arch/spinlock.h>

int pthread_spin_destroy(pthread_spinlock_t *lock) {
    if(!lock || *lock < 0)
        return EINVAL;

    *lock = -1;
    return 0;
}
