/* KallistiOS ##version##

   pthread_spin_lock.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <arch/spinlock.h>

int pthread_spin_lock(pthread_spinlock_t *lock) {
    spinlock_lock(lock);
    return 0;
}
