/* KallistiOS ##version##

   cnd_broadcast.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <threads.h>

int cnd_broadcast(cnd_t *cond) {
    return cond_broadcast(cond) ? thrd_error : thrd_success;
}
