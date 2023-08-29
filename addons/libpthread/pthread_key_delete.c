/* KallistiOS ##version##

   pthread_key_delete.c
   Copyright (C) 2011 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <kos/thread.h>

int pthread_key_delete(pthread_key_t key) {
    return kthread_key_delete(key);
}
