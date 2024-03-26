/* KallistiOS ##version##

   pthread_atfork.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_atfork(void (*prepare)(void), void (*parent)(void),
                   void (*child)(void)) {
    (void)prepare;
    (void)parent;
    (void)child;

    return ENOSYS;
}
