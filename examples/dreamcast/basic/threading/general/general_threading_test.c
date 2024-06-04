/* KallistiOS ##version##

   general_threading_test.c

   Copyright (C) 2000-2002 Megan Potter
   Copyright (C) 2024 Falco Girgis

   A simple thread example

   This small program shows off the threading (and also is used as
   a regression test to make sure threading is still approximately
   working =). See below for some more specific notes.

 */

#include <kos.h>

/* Semaphore used for timing below */
semaphore_t sem;

/* This routine will be started as thread #0 */
void *thd_0(void *v) {
    int x, y;

    printf("Thread 0 started\n");

    for(y = 0; y < 480; y++)
        for(x = 0; x < 320; x++)
            vram_s[y * 640 + x] = (((x * x) + (y * y)) & 0x1f) << 11;

    thd_pslist(printf);

    printf("Thread 0 finished\n");
    return NULL;
}

/* This routine will be started as thread #1 */
void *thd_1(void *v) {
    int i;

    for(i = 0; i < 30; i++) {
        printf("Hi from thread 1\n");
        thd_sleep(100);
        sem_signal(&sem);
    }

    printf("Thread 1 waiting:\n");
    thd_sleep(5000);

    thd_pslist(printf);

    printf("Thread 1 exiting\n");
    return NULL;
}

/* This routine will be started as thread #2 */
void *thd_2(void *v) {
    int i;

    thd_sleep(50);

    for(i = 0; i < 29; i++) {
        sem_wait(&sem);
        printf("Hi from thread 2\n");
    }

    printf("sem_wait_timed returns %d\n", sem_wait_timed(&sem, 200));
    printf("sem_wait_timed returns %d\n", sem_wait_timed(&sem, 200));
    printf("sem_wait_timed returns %d\n", sem_wait_timed(&sem, 200));

    thd_pslist(printf);

    printf("Thread 2 exiting\n");
    return NULL;
}

/* Condvar/mutex used for timing below */
mutex_t mut = MUTEX_INITIALIZER;
condvar_t cv = COND_INITIALIZER;
volatile int cv_ready = 0, cv_cnt = 0, cv_quit = 0;

/* This routine will be started N times for the condvar testing */
void *thd_3(void *v) {
    printf("Thread %d started\n", (int)v);

    mutex_lock(&mut);

    for(; ;) {
        while(!cv_ready && !cv_quit) {
            cond_wait(&cv, &mut);
        }

        if(!cv_quit) {
            printf("Thread %d re-activated. Count is now %d.\n",
                   (int)v, ++cv_cnt);
            cv_ready = 0;
        }
        else
            break;

    }

    mutex_unlock(&mut);

    printf("Thread %d exiting\n", (int)v);
    return NULL;
}

/* The main program */
int main(int argc, char **argv) {
    int x, y, i;
    kthread_t * t0, * t1, * t2, *t3[10];

    cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
                      (cont_btn_callback_t)arch_exit);

    /* Print a banner */
    printf("KOS 2.0.x thread program:\n");

    /* Create a semaphore for timing purposes */
    sem_init(&sem, 1);

    /* In the foreground, draw a moire pattern on the screen */
    for(y = 0; y < 480; y++)
        for(x = 320; x < 640; x++)
            vram_s[y * 640 + x] = ((x * x) + (y * y)) & 0x1f;

    /* Create two threads, which can/will start immediately. All threads are
       created as joinable so we can thd_join them later on to wait for them
       to die. */
    t0 = thd_create(0, thd_0, NULL);
    t1 = thd_create(0, thd_1, NULL);
    t2 = thd_create(0, thd_2, NULL);

    timer_spin_sleep(1000);
    thd_pslist(printf);
    thd_pslist_queue(printf);

    printf("Waiting for the death of thread 1:\n");
    x = thd_join(t1, NULL);

    printf("Retval was %d. Waiting for the death of thread 2:\n", x);
    x = thd_join(t2, NULL);
    printf("Retval was %d.\n", x);

    printf("Testing idle sleeping:\n");
    thd_pslist(printf);
    thd_sleep(1000);
    printf("Test succeeded\n");

    thd_join(t0, NULL);

    printf("\n\nCondvar test; starting threads\n");
    printf("Main thread is %p\n", thd_current);

    for(i = 0; i < 10; i++) {
        t3[i] = thd_create(0, thd_3, (void *)i);
        printf("Thread %d is %p\n", i, t3[i]);
    }

    thd_sleep(500);

    printf("\nOne-by-one test:\n");

    for(i = 0; i < 10; i++) {
        mutex_lock(&mut);
        cv_ready = 1;
        printf("Signaling %d:\n", i);
        cond_signal(&cv);
        mutex_unlock(&mut);
        thd_sleep(100);
    }

    printf("\nAgain, without waiting:\n");

    for(i = 0; i < 10; i++) {
        mutex_lock(&mut);
        cv_ready = 1;
        printf("Signaling %d:\n", i);
        cond_signal(&cv);
        mutex_unlock(&mut);
    }

    thd_sleep(100);
    printf("  (might not be the full 10)\n");

    printf("\nBroadcast test:\n");
    mutex_lock(&mut);
    cv_ready = 1;
    cond_broadcast(&cv);
    mutex_unlock(&mut);
    thd_sleep(100);
    printf("  (only one should have gotten through)\n");

    printf("\nKilling all condvar threads:\n");
    mutex_lock(&mut);
    cv_quit = 1;
    cond_broadcast(&cv);
    mutex_unlock(&mut);

    thd_pslist(printf);

    for(i = 0; i < 10; i++)
        thd_join(t3[i], NULL);

    /* Shut it all down */
    printf("Done.\n");

    return 0;
}
