/* KallistiOS ##version##

   oneshot_timer.c
   Copyright (C) 2024 Paul Cercueil
*/

#include <kos/genwait.h>
#include <kos/worker_thread.h>
#include <kos/oneshot_timer.h>
#include <stdlib.h>

struct oneshot_timer {
    kthread_worker_t *worker;
    void (*cb)(void *);
    void *data;
    unsigned int timeout_ms;
};

static void oneshot_timer_timeout(void *d) {
    oneshot_timer_t *timer = d;
    int ret;

    ret = genwait_wait(timer, "One-shot timer", timer->timeout_ms, NULL);
    if(ret < 0)
        timer->cb(timer->data);
}

void oneshot_timer_setup(oneshot_timer_t *timer, void (*cb)(void *),
                         void *data, unsigned int timeout_ms) {
    timer->timeout_ms = timeout_ms;
    timer->cb = cb;
    timer->data = data;
}

oneshot_timer_t *oneshot_timer_create(void (*cb)(void *), void *data,
                                      unsigned int timeout_ms) {
    oneshot_timer_t *timer = malloc(sizeof(*timer));
    if(!timer)
        return NULL;

    oneshot_timer_setup(timer, cb, data, timeout_ms);

    timer->worker = thd_worker_create(oneshot_timer_timeout, timer);
    if(!timer->worker) {
        free(timer);
        return NULL;
    }

    return timer;
}

void oneshot_timer_destroy(oneshot_timer_t *timer) {
    oneshot_timer_stop(timer);
    thd_worker_destroy(timer->worker);
    free(timer);
}

void oneshot_timer_start(oneshot_timer_t *timer) {
    thd_worker_wakeup(timer->worker);
}

void oneshot_timer_stop(oneshot_timer_t *timer) {
    genwait_wake_all(timer);
}
