/* KallistiOS ##version##

   rlock_test.c
   Copyright (C) 2008 Lawrence Sebald

*/

/* This program is a test for the recursive locks added in KOS 2.0.0. This
   synchronization primitive works essentially the same as a mutex, but allows
   the thread that owns the lock to acquire it as many times as it wants.

   Note that during the development of KOS 2.0.0, the recursive lock type got
   merged into the mutex type (when it was rewritten). */

#include <stdio.h>
#include <stdlib.h>

#include <kos/thread.h>
#include <kos/mutex.h>

#include <dc/maple.h>
#include <dc/maple/controller.h>

#define UNUSED __attribute__((unused))

mutex_t l = RECURSIVE_MUTEX_INITIALIZER;

void *thd0(void *param UNUSED) {
    int i;

    printf("Thd 0: About to obtain lock 10 times\n");

    for(i = 0; i < 10; ++i) {
        mutex_lock(&l);
    }

    printf("Thd 0: Lock acquired %d times\n", l.count);
    printf("Thd 0: About to sleep\n");
    thd_sleep(100);

    printf("Thd 0: Awake, about to release lock 9 times\n");

    for(i = 0; i < 9; ++i) {
        mutex_unlock(&l);
    }

    printf("Thd 0: About to sleep again\n");
    thd_sleep(10);

    printf("Thd 0: Awake, about to release lock\n");
    mutex_unlock(&l);
    printf("Thd 0: done\n");
    return NULL;
}

void *thd1(void *param UNUSED) {
    printf("Thd 1: About to obtain lock 2 times\n");
    mutex_lock(&l);
    mutex_lock(&l);

    printf("Thd 1: About to pass timeslice\n");
    thd_pass();

    printf("Thd 1: Awake, going to release lock 2 times\n");
    mutex_unlock(&l);
    mutex_unlock(&l);

    printf("Thd 1: About to obtain lock 1 time\n");
    mutex_lock(&l);

    printf("Thd 1: About to release lock\n");
    mutex_unlock(&l);
    printf("Thd 1: done\n");
    return NULL;
}

void *thd2(void *param UNUSED) {
    int i;

    printf("Thd 2: About to obtain lock 200 times\n");

    for(i = 0; i < 200; ++i) {
        mutex_lock(&l);
    }

    printf("Thd 2: About to release lock 200 times\n");

    for(i = 0; i < 200; ++i) {
        mutex_unlock(&l);
    }

    printf("Thd 2: done\n");
    return NULL;
}

int main(int argc, char *argv[]) {
    kthread_t *t0, *t1, *t2;

    /* Exit if the user presses all buttons at once. */
    cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
                      (cont_btn_callback_t)exit);

    printf("KallistiOS Recursive Lock test program\n");

    printf("About to create threads\n");
    t0 = thd_create(0, thd0, NULL);
    t1 = thd_create(0, thd1, NULL);
    t2 = thd_create(0, thd2, NULL);

    printf("About to sleep\n");
    thd_join(t0, NULL);
    thd_join(t1, NULL);
    thd_join(t2, NULL);

    if(mutex_is_locked(&l)) {
        printf("Lock is still locked!\n");
        exit(EXIT_FAILURE);
    }

    mutex_destroy(&l);

    printf("Recursive lock tests completed successfully!\n");
    return EXIT_SUCCESS;
}
