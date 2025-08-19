/* KallistiOS ##version##

   pthread_once.c
   Copyright (C) 2011 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <kos/once.h>

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void)) {
    return kthread_once(once_control, init_routine);
}
