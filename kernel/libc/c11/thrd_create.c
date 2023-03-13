/* KallistiOS ##version##

   thrd_create.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <threads.h>

typedef void *(*kthd_func)(void *);

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg) {
    kthread_t *thd = thd_create(0, (kthd_func)(void*)func, arg);

    if(!thd)
        return thrd_nomem;

    *thr = thd;
    return thrd_success;
}
