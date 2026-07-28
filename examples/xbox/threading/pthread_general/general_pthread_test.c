/* KallistiOS ##version##

   general_pthread_test.c
   Copyright (C) 2023 Lawrence Sebald

   Adapted from the Dreamcast pthread example for platforms without video or
   controller drivers. The pthread tests themselves remain architecture
   independent.
 */

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Condvar/mutex used for timing below. */
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
pthread_rwlock_t rw = PTHREAD_RWLOCK_INITIALIZER;
volatile int cv_ready = 0, cv_cnt = 0, cv_quit = 0;
volatile int filler_finished = 0;

void *filler_thd(void *v) {
    volatile uint32_t checksum = 0;
    int x, y;

    (void)v;
    printf("Filler thread started\n");

    for(y = 0; y < 480; y++)
        for(x = 0; x < 320; x++)
            checksum += (uint32_t)((x * x) + (y * y));

    assert(checksum != 0);
    __atomic_store_n(&filler_finished, 1, __ATOMIC_RELEASE);
    printf("Filler thread finished\n");
    return NULL;
}

void *mut_thd(void *v) {
    int r;
    printf("Thread %d: Started\n", (int)(intptr_t)v);

    assert(pthread_mutex_lock(&mut) == 0);
    printf("Thread %d: Acquired the lock\n", (int)(intptr_t)v);

    r = rand() % 5;
    printf("Thread %d: Sleeping for %d seconds\n", (int)(intptr_t)v, r);
    sleep((unsigned)r);
    printf("Thread %d: Woke up, releasing lock\n", (int)(intptr_t)v);

    assert(pthread_mutex_unlock(&mut) == 0);
    return NULL;
}

/* This routine will be started N times for the condvar testing. */
void *cv_thd(void *v) {
    printf("Thread %d started\n", (int)(intptr_t)v);

    assert(pthread_mutex_lock(&mut) == 0);

    for(;;) {
        while(!cv_ready && !cv_quit)
            assert(pthread_cond_wait(&cv, &mut) == 0);

        if(!cv_quit) {
            printf("Thread %d re-activated. Count is now %d.\n",
                   (int)(intptr_t)v, ++cv_cnt);
            cv_ready = 0;
        }
        else {
            break;
        }
    }

    assert(pthread_mutex_unlock(&mut) == 0);

    printf("Thread %d exiting\n", (int)(intptr_t)v);
    return NULL;
}

void *rd_thd(void *v) {
    int r;
    printf("Thread %d: Started\n", (int)(intptr_t)v);

    assert(pthread_rwlock_rdlock(&rw) == 0);
    printf("Thread %d: Acquired the read lock\n", (int)(intptr_t)v);

    r = rand() % 5;
    printf("Thread %d: Sleeping for %d seconds\n", (int)(intptr_t)v, r);
    sleep((unsigned)r);
    printf("Thread %d: Woke up, releasing read lock\n", (int)(intptr_t)v);

    assert(pthread_rwlock_unlock(&rw) == 0);
    return NULL;
}

void *wr_thd(void *v) {
    int r;
    printf("Thread %d: Started\n", (int)(intptr_t)v);

    assert(pthread_rwlock_wrlock(&rw) == 0);
    printf("Thread %d: Acquired the write lock\n", (int)(intptr_t)v);

    r = rand() % 3;
    printf("Thread %d: Sleeping for %d seconds\n", (int)(intptr_t)v, r);
    sleep((unsigned)r);
    printf("Thread %d: Woke up, releasing write lock\n", (int)(intptr_t)v);

    assert(pthread_rwlock_unlock(&rw) == 0);
    return NULL;
}

int main(int argc, char **argv) {
    int i;
    pthread_t cvt[10], filler;
    pthread_attr_t attr;

    (void)argc;
    (void)argv;

    printf("KOS pthread test program:\n");
    printf("Main thread is %p\n", (void *)pthread_self());

    printf("Creating detached filler thread\n");
    assert(pthread_attr_init(&attr) == 0);
    assert(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) == 0);
    assert(pthread_create(&filler, &attr, filler_thd, NULL) == 0);
    assert(pthread_attr_destroy(&attr) == 0);

    printf("Starting mutex test...\n");
    for(i = 0; i < 5; ++i) {
        assert(pthread_create(&cvt[i], NULL, mut_thd,
                              (void *)(intptr_t)i) == 0);
        printf("Thread %d is %p\n", i, (void *)cvt[i]);
    }

    printf("Waiting for threads to return...\n");
    for(i = 0; i < 5; i++)
        assert(pthread_join(cvt[i], NULL) == 0);

    printf("Completed mutex test...\n");

    printf("Starting condvar test...\n");
    for(i = 0; i < 10; ++i) {
        assert(pthread_create(&cvt[i], NULL, cv_thd,
                              (void *)(intptr_t)i) == 0);
        printf("Thread %d is %p\n", i, (void *)cvt[i]);
    }

    usleep(500 * 1000);
    printf("\nOne-by-one test:\n");

    for(i = 0; i < 10; i++) {
        assert(pthread_mutex_lock(&mut) == 0);
        cv_ready = 1;
        printf("Signaling %d:\n", i);
        assert(pthread_cond_signal(&cv) == 0);
        assert(pthread_mutex_unlock(&mut) == 0);
        usleep(100 * 1000);
    }

    printf("\nAgain, without waiting:\n");
    for(i = 0; i < 10; i++) {
        assert(pthread_mutex_lock(&mut) == 0);
        cv_ready = 1;
        printf("Signaling %d:\n", i);
        assert(pthread_cond_signal(&cv) == 0);
        assert(pthread_mutex_unlock(&mut) == 0);
    }

    usleep(100 * 1000);
    printf("  (might not be the full 10)\n");

    printf("\nBroadcast test:\n");
    assert(pthread_mutex_lock(&mut) == 0);
    cv_ready = 1;
    assert(pthread_cond_broadcast(&cv) == 0);
    assert(pthread_mutex_unlock(&mut) == 0);
    usleep(100 * 1000);
    printf("  (only one should have gotten through)\n");

    printf("\nKilling all condvar threads:\n");
    assert(pthread_mutex_lock(&mut) == 0);
    cv_quit = 1;
    assert(pthread_cond_broadcast(&cv) == 0);
    assert(pthread_mutex_unlock(&mut) == 0);

    printf("Waiting for threads to return...\n");
    for(i = 0; i < 10; i++)
        assert(pthread_join(cvt[i], NULL) == 0);

    printf("Completed condvar test...\n");

    printf("Starting rwlock test...\n");
    for(i = 0; i < 10; ++i) {
        void *argument = (void *)(intptr_t)i;
        void *(*routine)(void *) = i % 2 ? rd_thd : wr_thd;

        assert(pthread_create(&cvt[i], NULL, routine, argument) == 0);
        printf("Thread %d (%s) is %p\n", i, i % 2 ? "read" : "write",
               (void *)cvt[i]);
    }

    printf("Waiting for threads to return...\n");
    for(i = 0; i < 10; i++)
        assert(pthread_join(cvt[i], NULL) == 0);

    printf("Completed rwlock test...\n");

    while(!__atomic_load_n(&filler_finished, __ATOMIC_ACQUIRE))
        usleep(1000);

    printf("PTHREAD TEST SUCCESS\n");
    return 0;
}
