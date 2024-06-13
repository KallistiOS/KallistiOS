/* KallistiOS ##version##

   pthread_setspecific.c
   Copyright (C) 2011 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <kos/thread.h>

int pthread_setspecific(pthread_key_t key, const void *value) {
    return kthread_setspecific(key, value);
}
