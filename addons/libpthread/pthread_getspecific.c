/* KallistiOS ##version##

   pthread_getspecific.c
   Copyright (C) 2011 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <kos/thread.h>

void *pthread_getspecific(pthread_key_t key) {
    return kthread_getspecific(key);
}
