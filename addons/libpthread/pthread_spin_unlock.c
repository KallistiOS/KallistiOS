/* KallistiOS ##version##

   pthread_spin_unlock.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <arch/spinlock.h>

int pthread_spin_unlock(pthread_spinlock_t *lock) {
    spinlock_unlock(lock);
    return 0;
}
