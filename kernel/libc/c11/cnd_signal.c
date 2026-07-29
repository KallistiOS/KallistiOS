/* KallistiOS ##version##

   cnd_signal.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <threads.h>

int cnd_signal(cnd_t *cond) {
    return cond_signal(cond) ? thrd_error : thrd_success;
}
