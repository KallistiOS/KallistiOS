/* KallistiOS ##version##

   tss_set.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <threads.h>

int tss_set(tss_t key, void *val) {
    return kthread_setspecific(key, val) ? thrd_error : thrd_success;
}
