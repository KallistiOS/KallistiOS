/* KallistiOS ##version##

   pthread_exit.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <kos/thread.h>

void pthread_exit(void *value_ptr) {
    thd_exit(value_ptr);
}
