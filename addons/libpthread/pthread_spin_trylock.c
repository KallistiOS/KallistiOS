/* KallistiOS ##version##

   pthread_spin_trylock.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <arch/spinlock.h>

int pthread_spin_trylock(pthread_spinlock_t *lock) {
    int gotlock = spinlock_trylock(lock);

    if(!gotlock)
        return EBUSY;

    return 0;
}
