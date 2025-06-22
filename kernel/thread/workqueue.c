/* KallistiOS ##version##

   workqueue.c
   Copyright (C) 2026 Paul Cercueil
*/

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/queue.h>

#include <kos/cond.h>
#include <kos/mutex.h>
#include <kos/timer.h>
#include <kos/thread.h>
#include <kos/workqueue.h>

typedef struct workqueue {
    STAILQ_HEAD(workqueue_jobs, workqueue_job) jobs;
    kthread_t *thd;
    mutex_t lock;
    condvar_t cond;
    bool quit;
} workqueue_t;

static void *workqueue_thread(void *d) {
    workqueue_t *wq = d;
    workqueue_job_t *job;
    uint64_t now;
    int ret;

    while(!wq->quit) {
        mutex_lock(&wq->lock);

        job = STAILQ_FIRST(&wq->jobs);
        if(job)
            now = timer_ms_gettime64();

        if(!job || job->time_ms > now) {
            ret = cond_wait_timed(&wq->cond, &wq->lock,
                                  job ? job->time_ms - now : 0);
            if (!ret) {
                mutex_unlock(&wq->lock);

                /* We did not time out, so something was added to the queue. */
                continue;
            }
        }

        /* Remove the job from the queue */
        STAILQ_REMOVE(&wq->jobs, job, workqueue_job, entry);

        mutex_unlock(&wq->lock);

        job->cb(d, job);
    }

    return NULL;
}

static const kthread_attr_t workqueue_attrs = {
    .label = "[workqueue]",
};

workqueue_t *workqueue_create(void) {
    workqueue_t *wq;

    wq = calloc(1, sizeof(workqueue_t));
    if(!wq)
        return NULL;

    wq->lock = (mutex_t)MUTEX_INITIALIZER;
    wq->cond = (condvar_t)COND_INITIALIZER;
    STAILQ_INIT(&wq->jobs);

    wq->thd = thd_create_ex(&workqueue_attrs, workqueue_thread, wq);
    if(!wq->thd) {
        free(wq);
        return NULL;
    }

    return wq;
}

void workqueue_enqueue(workqueue_t *wq, workqueue_job_t *job) {
    workqueue_job_t *elm, *prev = NULL;

    mutex_lock_scoped(&wq->lock);

    if(!job->time_ms)
        job->time_ms = timer_ms_gettime64();

    STAILQ_FOREACH(elm, &wq->jobs, entry) {
        if(job->time_ms < elm->time_ms) {
            if(prev)
                STAILQ_INSERT_AFTER(&wq->jobs, prev, job, entry);
            else
                STAILQ_INSERT_HEAD(&wq->jobs, job, entry);
            break;
        }

        prev = elm;
    }

    if(!elm)
        STAILQ_INSERT_TAIL(&wq->jobs, job, entry);

    cond_signal(&wq->cond);
}

void workqueue_cancel(workqueue_t *wq, workqueue_job_t *job) {
    mutex_lock_scoped(&wq->lock);

    STAILQ_REMOVE(&wq->jobs, job, workqueue_job, entry);
    cond_signal(&wq->cond);
}

void workqueue_kill(workqueue_t *wq) {
    if(!wq->quit) {
        wq->quit = true;
        cond_signal(&wq->cond);
        thd_join(wq->thd, NULL);
    }
}

void workqueue_destroy(workqueue_t *wq) {
    workqueue_kill(wq);
    free(wq);
}

kthread_t *workqueue_get_thread(workqueue_t *wq) {
    return wq->thd;
}
