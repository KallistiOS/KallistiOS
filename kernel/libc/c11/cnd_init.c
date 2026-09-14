/* KallistiOS ##version##

   cnd_init.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <threads.h>

int cnd_init(cnd_t *cond) {
    return cond_init(cond) ? thrd_error : thrd_success;
}
