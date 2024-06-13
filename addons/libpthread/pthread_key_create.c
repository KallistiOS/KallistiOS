/* KallistiOS ##version##

   pthread_key_create.c
   Copyright (C) 2011 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <kos/thread.h>

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *)) {
    return kthread_key_create(key, destructor);
}
