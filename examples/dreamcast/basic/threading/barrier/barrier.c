/*  KallistiOS ##version##

    barrier.c

    Copyright (c) 2023 Falco Girgis

    kthread Barrier Example and Test

    This is a small program that serves as an example of and test for KOS's
    thread barrier API. It simply spawns a configurable amount of threads
    which it then passes through a pipeline of a configurable number of
    barriers a configurable number of times, incrementing counters which
    are later used to verify proper control flow. The watchdog timer is
    used to protect against any sort of deadlock should the test fail.

 */

#include <kos/thread.h>
#include <kos/barrier.h>
#include <arch/wdt.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdio.h>

/* Configurable constants */
#define WATCHDOG_TIMEOUT    (10 * 1000 * 1000) /* 10s */
#define THREAD_COUNT        15   /* Number of threads to spawn */
#define BARRIER_COUNT       5    /* Number of barriers within pipeline */
#define ITERATION_COUNT     10   /* Number of times to run through pipeline */

/* Our test data for the barrier pipeline */
static struct {
    /* KOS barrier */  
    thd_barrier_t barrier;                 
    /* Thread control-flow counters */ 
    atomic_uint   pre_barrier_counter;    /* All threads entering the barrier */    
    atomic_uint   serial_barrier_counter; /* Serial threads exiting the barrier */
    atomic_uint   post_barrier_counter;   /* All threads exiting the barrier */
} data[BARRIER_COUNT] = { 0 };

/*  Runs the current thread through a single iteration of the 
    barrier pipeline, incrementing counters as each barrier is
    approached and exited. */
static bool run_iteration(uintptr_t tid) {
    bool success = true;

    /* Go through every barrier sequentially. */
    for(unsigned b = 0; b < BARRIER_COUNT; ++b) {
        ++data[b].pre_barrier_counter;

        printf("Thread[%u]: Before barrier[%u]!\n", tid, b);

        /* Wait for the barrier. */
        const int ret = thd_barrier_wait(&data[b].barrier);

        /* Check for errors. */
        if(ret < 0) {
            fprintf(stderr, "Thread[%u]: Wait on barrier[%u] failure: %d!\n",
                    tid, b, ret);
            success = false;
        }
        /* Check if we were the lucky serial thread (should only be one!) */
        else if(ret == THD_BARRIER_SERIAL_THREAD) {
            printf("Thread[%u]: After barrier[%u]: SERIAL!\n", tid, b);
            ++data[b].serial_barrier_counter;
        }
        else { 
            printf("Thread[%u]: After barrier[%u]: NONSERIAL!\n", tid, b);
        }

        ++data[b].post_barrier_counter;
    }

    return success;
}

/*  Entry-point for each thread. Simply runs the thread through
    the pipeline barrier the configured number of iterations. */
static void *thread_exec(void *user_data) {
    bool success = true;
    uintptr_t tid = (uintptr_t)user_data;

    /* Accumulate result code after each pipeline pass. */
    for(unsigned i = 0; i < ITERATION_COUNT; ++i) 
        success &= run_iteration(tid);

    return (void*)success;
}

/* WDT callback for test timeout failure */
static void watchdog_timeout(void *user_data) {
    (void)user_data;

    fprintf(stderr, "\n**** FAILURE: Watchdog timeout reached! ****\n\n");
    exit(EXIT_FAILURE);
}

/* Program entry-point */
int main(int argc, char* argv[]) { 
    kthread_t *threads[THREAD_COUNT - 1];
    bool success = true;

    printf("Initializing Watchdog timer...\n");
    wdt_enable_timer(0, WATCHDOG_TIMEOUT, 0xf, 
                     watchdog_timeout, NULL);
    atexit(wdt_disable);

    printf("Creating %u barriers...\n", BARRIER_COUNT);
    for(unsigned b = 0; b < BARRIER_COUNT; ++b) { 
        const int ret = thd_barrier_init(&data[b].barrier, 
                                         NULL, THREAD_COUNT);

        if(ret) {
            fprintf(stderr, "Failed to create barrier[%d]: %d\n", b, ret);
            success = false;
        }
    }

    printf("Spawning %u threads...\n", THREAD_COUNT - 1);
    for(unsigned t = 0; t < THREAD_COUNT - 1; ++t) { 
        threads[t] = thd_create(false, thread_exec, (void *)(t + 1));

        if(!threads[t]) {
            fprintf(stderr, "Failed to create thread %u!", t  + 1);
            success = false;
        }
    }

    printf("Executing logic from main thread...\n");
    thread_exec((void *)0);

    printf("Joining threads...\n");
    for(unsigned  t = 0; t < THREAD_COUNT - 1; ++t) {
        void *thread_ret;
        const int ret = thd_join(threads[t], &thread_ret);

        if(ret) {
            fprintf(stderr, "Failed to join thread %u with code: %d!\n", 
                    t, ret);
            success = false;
        }
        else if(!thread_ret) {
            fprintf(stderr, "Thread %u returned an error!\n", t + 1);
            success = false;
        }
        else {
            printf("Thread %u completed successfully!\n", t + 1);
        }
    }

    printf("Verifying counters...\n");
    for(unsigned b = 0; b < BARRIER_COUNT; ++b) {
        if(data[b].pre_barrier_counter != THREAD_COUNT * ITERATION_COUNT) {
            fprintf(stderr, "Incorrect pre_barrier_counter[%u] - %u "
                    "(%u expected)!\n", b, data[b].pre_barrier_counter,
                    THREAD_COUNT * ITERATION_COUNT);
            success = false;
        }

        if(data[b].pre_barrier_counter != THREAD_COUNT * ITERATION_COUNT) {
            fprintf(stderr, "Incorrect post_barrier_counter[%u] - %u "
                    "(%u expected)!\n", b, data[b].post_barrier_counter,
                    THREAD_COUNT * ITERATION_COUNT);
            success = false;
        }

        if(data[b].serial_barrier_counter != ITERATION_COUNT) {
            fprintf(stderr, "Incorrect serial_barrier_counter[%u] - %u "
                    "(%u expected)!\n", b, data[b].serial_barrier_counter,
                    ITERATION_COUNT);
            success = false;
        }
    }

    printf("Destroying barriers...\n");
    for(unsigned b = 0; b < BARRIER_COUNT; ++b) { 
        const int ret = thd_barrier_destroy(&data[b].barrier);
        
        if(ret) {
            fprintf(stderr, "Failed to destroy barrier[%d]: %d!\n", b, ret);
            success = false;
        }
    }

    if(success) {
        printf("\n***** TEST COMPLETE: SUCCESS *****\n\n");
        return EXIT_SUCCESS;
    }
    else {
        fprintf(stderr, "\nXXXXX TEST COMPLETE: FAILURE XXXXX\n\n");
        return EXIT_FAILURE;
    }
}