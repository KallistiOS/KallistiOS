/* KallistiOS ##version##

   pthread_equal.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>

int pthread_equal(pthread_t t1, pthread_t t2) {
    return t1 == t2;
}
