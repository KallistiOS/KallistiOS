/* KallistiOS ##version##

   pthread_exit.c
   Copyright (C) 2023 Lawrence Sebald
   Copyright (C) 2026 Paul Cercueil
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <kos/thread.h>

struct pthread_cleanup_cb {
    void (*cb)(void *d);
    void *arg;
};

static _Thread_local struct pthread_cleanup_cb cleanups[16];
static _Thread_local unsigned int nb_cleanups;

void pthread_cleanup_push(void (*routine)(void *), void *arg) {
    if(nb_cleanups < __array_size(cleanups))
        cleanups[nb_cleanups++] = (struct pthread_cleanup_cb){ routine, arg };
}

void pthread_cleanup_pop(int execute) {
    if(nb_cleanups > 0) {
        nb_cleanups--;

        if(execute)
            cleanups[nb_cleanups].cb(cleanups[nb_cleanups].arg);
    }
}

void pthread_exit(void *value_ptr) {
    while(nb_cleanups > 0)
        pthread_cleanup_pop(1);

    thd_exit(value_ptr);
}
