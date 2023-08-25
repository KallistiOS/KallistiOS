/* KallistiOS ##version##

   once_test.c
   Copyright (C) 2009, 2023 Lawrence Sebald

*/

/* This program is a test for the kthread_once_t type added in KOS 2.0.0. A once
   object is used with the kthread_once function to ensure that an initializer
   function is only run once in a program (meaning multiple threads will not run
   the function. */

#include <stdio.h>
#include <kos/thread.h>
#include <kos/once.h>

#include <arch/arch.h>
#include <arch/spinlock.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

#define UNUSED __attribute__((unused))
#define THD_COUNT 600

kthread_once_t once = KTHREAD_ONCE_INIT;
spinlock_t lock = SPINLOCK_INITIALIZER;
int counter = 0;

void once_func(void) {
    spinlock_lock(&lock);
    ++counter;
    spinlock_unlock(&lock);
}

void *thd_func(void *param UNUSED) {
    kthread_t *cur = thd_get_current();

    printf("Thd %d: Attempting to call kthread_once\n", cur->tid);
    kthread_once(&once, &once_func);
    printf("Thd %d: kthread_once returned\n", cur->tid);
    return NULL;
}

KOS_INIT_FLAGS(INIT_DEFAULT);

int main(int argc, char *argv[]) {
    int i;
    kthread_t *thds[THD_COUNT];

    cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
                      (cont_btn_callback_t)arch_exit);

    printf("KallistiOS kthread_once test program\n");

    /* Create the threads. */
    printf("Creating %d threads\n", THD_COUNT);

    for(i = 0; i < THD_COUNT; ++i) {
        thds[i] = thd_create(0, &thd_func, NULL);
    }

    printf("Waiting for the threads to finish\n");

    for(i = 0; i < THD_COUNT; ++i) {
        thd_join(thds[i], NULL);
    }

    printf("Final counter value: %d (expected 1)\n", counter);
    printf("Test finished\n");

    return 0;
}
