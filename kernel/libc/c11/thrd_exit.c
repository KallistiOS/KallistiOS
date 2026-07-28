/* KallistiOS ##version##

   thrd_exit.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <threads.h>
#include <stdint.h>

_Noreturn void thrd_exit(int res) {
    thd_exit((void *)(intptr_t)res);
}
