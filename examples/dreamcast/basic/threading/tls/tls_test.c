/* KallistiOS ##version##

   tls_test.c
   Copyright (C) 2009 Lawrence Sebald
   Copyright (C) 2024 Falco Girgis

*/

/* This program is a test for thread-local storage added in KOS 1.3.0. */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <errno.h>
#include <string.h>

#include <kos/thread.h>
#include <kos/once.h>
#include <kos/tls.h>

#include <arch/arch.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

#define THREAD_COUNT 100

#define UNUSED __attribute__((unused))

static kthread_once_t once = KTHREAD_ONCE_INIT;
static kthread_key_t key1, key2;
static atomic_uint dtor_counter;

static void destructor(void *data) {
    printf("Destroying %d\n", (int)data);
    ++dtor_counter;
}

static void once_func(void) {
    if(kthread_key_create(&key2, &destructor)) {
        fprintf(stderr, "Error in calling kthread_key_create\n");
    }
}

static void *thd_func(void *param UNUSED) {
    kthread_t *cur = thd_get_current();
    void *data;
    int retval;

    printf("Thd %d: Reading key 1\n", cur->tid);
    data = kthread_getspecific(key1);

    printf("Thd %d: kthread_getspecific returned %p (should be NULL)\n",
           cur->tid, data);

    if(data) {
        fprintf(stderr, "Unexpected key 1 value: %p\n", data);
        thd_exit((void *)false);
    }

    printf("Thd %d: Will create key 2, if its not created\n", cur->tid);

    if((retval = kthread_once(&once, &once_func))) {
        fprintf(stderr, "kthread_once failed with %s!\n", strerror(errno));
        thd_exit((void *)false);
    }

    printf("Thd %d: Writing to key 2\n", cur->tid);

    if(kthread_setspecific(key2, (void *)cur->tid)) {
        fprintf(stderr, "Error in kthread_setspecific!!!\n");
        thd_exit((void *)false);
    }

    if(cur->tid & 0x01) {
        printf("Thd %d: sleeping...\n", cur->tid);
        thd_sleep(200);
    }

    printf("Thd %d: Reading key 2\n", cur->tid);
    data = kthread_getspecific(key2);
    printf("Thd %d: kthread_getspecific returned %d (should be %d)\n", cur->tid,
           (int)data, cur->tid);

    if((int)data != cur->tid) {
        fprintf(stderr, "Invalid value for key2: %p\n", data);
        thd_exit((void *)false);
    }

    return (void *)true;
}

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

int main(int argc, char *argv[]) {
    kthread_t *thds[THREAD_COUNT];
    void *data;
    bool success = true;
    int retval;

    cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
                      (cont_btn_callback_t)arch_exit);

    printf("KallistiOS TLS test program\n\n");

    printf("Main thread: Creating key 1\n");

    if(kthread_key_create(&key1, NULL)) {
        fprintf(stderr, "Error in creating key 1\n");
        exit(EXIT_FAILURE);
    }

    printf("Main thread: Setting key 1 to 0xDEADBEEF\n");

    if((retval = kthread_setspecific(key1, (void *)0xDEADBEEF))) {
        fprintf(stderr, "kthread_setspecific() failed: %d\n", retval);
        exit(EXIT_FAILURE);
    }

    data = kthread_getspecific(key1);
    printf("Main thread: Key 1 value: %p\n", data);

    if(data != (void *)0xDEADBEEF) {
        fprintf(stderr, "Unexpected kthread_getspecific() value: %p\n", data);
        exit(EXIT_FAILURE);
    }

    /* Create the threads. */
    printf("Main thread: Creating %d threads\n", THREAD_COUNT);
    for(int t = 0; t < THREAD_COUNT; ++t)
        thds[t] = thd_create(0, &thd_func, NULL);

    printf("Main thread: Waiting for the threads to finish\n");
    for(int t = 0; t < THREAD_COUNT; ++t) {
        thd_join(thds[t], &data);
        success &= (bool)data;
    }

    if(!success) {
        fprintf(stderr, "Test failed!\n");
        exit(EXIT_FAILURE);
    }

    if(dtor_counter != THREAD_COUNT) {
        fprintf(stderr, "Incorrect destructor counter value: %u\n",
                dtor_counter);
        exit(EXIT_FAILURE);
    } else {
        printf("Correct destructor counter value: %u\n", dtor_counter);
    }

    data = kthread_getspecific(key1);
    printf("Main thread: Key 1 value: %p\n", data);

    if(data != (void *)0xDEADBEEF) {
        fprintf(stderr, "Unexpected final kthread_getspecific() value: %p\n", data);
        exit(EXIT_FAILURE);
    }

    printf("Main thread: Removing keys\n");

    success &= !kthread_key_delete(key1);
    success &= !kthread_key_delete(key2);

    if(!success) {
        fprintf(stderr, "Failed to delete kthread keys!\n");
        exit(EXIT_FAILURE);
    }

    printf("\n===== TLS TEST SUCCESS =====\n");

    return EXIT_SUCCESS;
}
