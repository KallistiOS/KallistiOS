/* KallistiOS ##version##

   once_test.c
   Copyright (C) 2009, 2023 Lawrence Sebald

*/

/* This program is a test for the kthread_once_t type added in KOS 2.0.0. A once
   object is used with the kthread_once function to ensure that an initializer
   function is only run once in a program (meaning multiple threads will not run
   the function. */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <kos/thread.h>
#include <kos/once.h>

#include <arch/arch.h>
#include <arch/spinlock.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

#define UNUSED __attribute__((unused))
#define THD_COUNT (475 * (DBL_MEM? 2 : 1))

static kthread_once_t once = KTHREAD_ONCE_INIT;
static spinlock_t lock = SPINLOCK_INITIALIZER;
static int counter = 0;

static void inner_once_func(void) {
    spinlock_lock(&lock);
    ++counter;
    spinlock_unlock(&lock);
}

static void *inner_thd_func(void *param UNUSED) {
    static kthread_once_t inner_once = KTHREAD_ONCE_INIT;
    const kthread_t *cur = thd_get_current();

    printf("Thd %d: Attempting to call inner kthread_once\n", cur->tid);
    kthread_once(&inner_once, &inner_once_func);    
    printf("Thd %d: inner kthread_once returned\n", cur->tid);
    
    return NULL;
}

static void once_func(void) {
    const kthread_t *cur = thd_get_current();

    printf("Thd %d: Spawning subthread\n", cur->tid);
    kthread_t *subthd = thd_create(0, &inner_thd_func, NULL);
    thd_join(subthd, NULL);
    printf("Thd %d: Joined subthread\n", cur->tid);
}

static void *thd_func(void *param UNUSED) {
    const kthread_t *cur = thd_get_current();

    printf("Thd %d: Attempting to call kthread_once\n", cur->tid);
    kthread_once(&once, &once_func);
    printf("Thd %d: kthread_once returned\n", cur->tid);
    return NULL;
}

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

int main(int argc, char *argv[]) {
    int i, retval, success = 1;
    kthread_t *thds[THD_COUNT];

    cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
                      (cont_btn_callback_t)arch_exit);

    printf("KallistiOS kthread_once test program\n");

    /* Create the threads. */
    printf("Creating %d threads\n", THD_COUNT);

    for(i = 0; i < THD_COUNT; ++i) {
        thds[i] = thd_create(0, &thd_func, NULL);
        
        if(!thds[i]) {
            fprintf(stderr, "Failed to spawn thread[%d]: %s\n",
                    i, strerror(errno));
            success = 0;
        }
    }

    printf("Waiting for the threads to finish\n");

    for(i = 0; i < THD_COUNT; ++i) {
        if((retval = thd_join(thds[i], NULL)) < 0) {
            fprintf(stderr, "Failed to join thread[%d]: %d\n", 
                    i, retval);
            success = 0;
        }
    }

    printf("Final counter value: %d (expected 1)\n\n", counter);

    if(success && counter == 1) {
        printf("***** ONCE_TEST PASSED *****\n");
        return EXIT_SUCCESS;
    }
    else {
        fprintf(stderr, "***** ONCE_TEST FAILED *****\n");
        return EXIT_FAILURE;
    }
}
