/* KallistiOS ##version##

   pthread_spin_initdestroy.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <arch/spinlock.h>

int pthread_spin_init(pthread_spinlock_t *lock, int pshared) {
    (void)pshared;

    if(!lock)
        return EFAULT;

    *lock = SPINLOCK_INITIALIZER;
    return 0;
}
