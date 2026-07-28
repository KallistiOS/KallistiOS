/* KallistiOS ##version##

   thrd_create.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <threads.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct thrd_start_context {
    thrd_start_t func;
    void *arg;
} thrd_start_context_t;

static void *thrd_start(void *arg) {
    thrd_start_context_t *context = arg;
    thrd_start_t func = context->func;
    void *func_arg = context->arg;
    int result;

    free(context);
    result = func(func_arg);
    return (void *)(intptr_t)result;
}

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg) {
    thrd_start_context_t *context = malloc(sizeof(*context));
    kthread_t *thd;

    if(!context)
        return thrd_nomem;

    context->func = func;
    context->arg = arg;
    thd = thd_create(0, thrd_start, context);
    if(!thd) {
        free(context);
        return thrd_nomem;
    }

    *thr = thd;
    return thrd_success;
}
