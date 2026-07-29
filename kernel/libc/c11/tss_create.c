/* KallistiOS ##version##

   tss_create.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <threads.h>

int tss_create(tss_t *key, tss_dtor_t dtor) {
    return kthread_key_create(key, dtor) ? thrd_error : thrd_success;
}
