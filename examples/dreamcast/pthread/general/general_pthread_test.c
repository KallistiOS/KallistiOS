/* KallistiOS ##version##

   general_pthread_test.c
   Copyright (C) 2023 Lawrence Sebald

   Adapted from:

   general_threading_test.c
   Copyright (C) 2000-2002 Megan Potter

   A simple thread example

   This small program shows off the threading (and also is used as
   a regression test to make sure threading is still approximately
   working =). See below for some more specific notes.

 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <arch/arch.h>
#include <dc/video.h>
#include <dc/maple/controller.h>

/* Condvar/mutex used for timing below */
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
volatile int cv_ready = 0, cv_cnt = 0, cv_quit = 0;

/* This routine will be started N times for the condvar testing */
void *cv_thd(void *v) {
    printf("Thread %d started\n", (int)v);

    pthread_mutex_lock(&mut);

    for(; ;) {
        while(!cv_ready && !cv_quit) {
            pthread_cond_wait(&cv, &mut);
        }

        if(!cv_quit) {
            printf("Thread %d re-activated. Count is now %d.\n",
                   (int)v, ++cv_cnt);
            cv_ready = 0;
        }
        else
            break;

    }

    pthread_mutex_unlock(&mut);

    printf("Thread %d exiting\n", (int)v);
    return NULL;
}

/* The main program */
int main(int argc, char **argv) {
    int x, y, i;
    pthread_t cvt[10];

    cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
                      (cont_btn_callback_t)arch_exit);

    /* Print a banner */
    printf("KOS pthread test program:\n");

    /* In the foreground, draw a moire pattern on the screen */
    for(y = 0; y < 480; y++)
        for(x = 320; x < 640; x++)
            vram_s[y * 640 + x] = ((x * x) + (y * y)) & 0x1f;

    printf("\n\nCondvar test; starting threads\n");
    printf("Main thread is %p\n", (void *)pthread_self());

    for(i = 0; i < 10; i++) {
        assert(pthread_create(&cvt[i], NULL, cv_thd, (void *)i));
        printf("Thread %d is %p\n", i, (void *)cvt[i]);
    }

    usleep(500 * 1000);

    printf("\nOne-by-one test:\n");

    for(i = 0; i < 10; i++) {
        pthread_mutex_lock(&mut);
        cv_ready = 1;
        printf("Signaling %d:\n", i);
        pthread_cond_signal(&cv);
        pthread_mutex_unlock(&mut);
        usleep(100 * 1000);
    }

    printf("\nAgain, without waiting:\n");

    for(i = 0; i < 10; i++) {
        pthread_mutex_lock(&mut);
        cv_ready = 1;
        printf("Signaling %d:\n", i);
        pthread_cond_signal(&cv);
        pthread_mutex_unlock(&mut);
    }

    usleep(100 * 1000);
    printf("  (might not be the full 10)\n");

    printf("\nBroadcast test:\n");
    pthread_mutex_lock(&mut);
    cv_ready = 1;
    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&mut);
    usleep(100 * 1000);
    printf("  (only one should have gotten through)\n");

    printf("\nKilling all condvar threads:\n");
    pthread_mutex_lock(&mut);
    cv_quit = 1;
    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&mut);

    for(i = 0; i < 10; i++)
        pthread_join(cvt[i], NULL);

    /* Shut it all down */
    printf("Done.\n");

    return 0;
}
