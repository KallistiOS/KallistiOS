/* KallistiOS ##version##

   workqueue.c
   Copyright (C) 2026 Paul Cercueil
   Copyright (C) 2026 Joseph Black
*/

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/queue.h>

#include <kos/cond.h>
#include <kos/mutex.h>
#include <kos/timer.h>
#include <kos/thread.h>
#include <kos/workqueue.h>

typedef struct workqueue_cancellation {
    workqueue_job_t *job;
    STAILQ_ENTRY(workqueue_cancellation) entry;
} workqueue_cancellation_t;

typedef struct workqueue {
    STAILQ_HEAD(workqueue_jobs, workqueue_job) jobs;
    STAILQ_HEAD(workqueue_cancellations, workqueue_cancellation) cancellations;
    workqueue_job_t *curr_job;
    kthread_t *thd;
    mutex_t lock;
    condvar_t cond;
    bool quit;
    bool joining;
    bool joined;
} workqueue_t;

static bool job_is_queued(const workqueue_t *wq,
                          const workqueue_job_t *job) {
    const workqueue_job_t *queued;

    STAILQ_FOREACH(queued, &wq->jobs, entry) {
        if(queued == job)
            return true;
    }

    return false;
}

static bool remove_queued_job(workqueue_t *wq, workqueue_job_t *job) {
    if(!job_is_queued(wq, job))
        return false;

    STAILQ_REMOVE(&wq->jobs, job, workqueue_job, entry);
    return true;
}

static bool job_is_cancelling(const workqueue_t *wq,
                              const workqueue_job_t *job) {
    const workqueue_cancellation_t *cancellation;

    STAILQ_FOREACH(cancellation, &wq->cancellations, entry) {
        if(cancellation->job == job)
            return true;
    }

    return false;
}

static int deadline_timeout(uint64_t deadline, uint64_t now) {
    uint64_t remaining;

    if(deadline <= now)
        return 1;

    remaining = deadline - now;
    return remaining > INT_MAX ? INT_MAX : (int)remaining;
}

static void *workqueue_thread(void *d) {
    workqueue_t *wq = d;
    workqueue_job_t *job;
    uint64_t now;

    for(;;) {
        mutex_lock(&wq->lock);

        /* Signal that we're done with the previous job */
        wq->curr_job = NULL;
        cond_broadcast(&wq->cond);

        for(;;) {
            if(wq->quit) {
                mutex_unlock(&wq->lock);
                return NULL;
            }

            job = STAILQ_FIRST(&wq->jobs);
            if(!job) {
                (void)cond_wait(&wq->cond, &wq->lock);
                continue;
            }

            now = timer_ms_gettime64();
            if(job->time_ms <= now)
                break;

            (void)cond_wait_timed(&wq->cond, &wq->lock,
                                  deadline_timeout(job->time_ms, now));
        }

        STAILQ_REMOVE_HEAD(&wq->jobs, entry);

        wq->curr_job = job;
        mutex_unlock(&wq->lock);

        job->cb(d, job);
    }
}

static const kthread_attr_t workqueue_attrs = {
    .label = "[workqueue]",
};

workqueue_t *workqueue_create(void) {
    workqueue_t *wq;

    if(irq_inside_int()) {
        errno = EPERM;
        return NULL;
    }

    wq = calloc(1, sizeof(workqueue_t));
    if(!wq)
        return NULL;

    wq->lock = (mutex_t)MUTEX_INITIALIZER;
    wq->cond = (condvar_t)COND_INITIALIZER;
    STAILQ_INIT(&wq->jobs);
    STAILQ_INIT(&wq->cancellations);

    wq->thd = thd_create_ex(&workqueue_attrs, workqueue_thread, wq);
    if(!wq->thd) {
        const int saved_errno = errno;
        cond_destroy(&wq->cond);
        mutex_destroy(&wq->lock);
        free(wq);
        errno = saved_errno;
        return NULL;
    }

    return wq;
}

int workqueue_enqueue_ex(workqueue_t *wq, workqueue_job_t *job) {
    workqueue_job_t *elm, *prev = NULL;

    if(irq_inside_int()) {
        errno = EPERM;
        return -1;
    }
    if(!wq || !job || !job->cb) {
        errno = EINVAL;
        return -1;
    }

    mutex_lock_scoped(&wq->lock);

    if(wq->quit || job_is_cancelling(wq, job)) {
        errno = ECANCELED;
        return -1;
    }
    if(job_is_queued(wq, job)) {
        errno = EBUSY;
        return -1;
    }

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

    /* Cancellation and shutdown waiters share this condition with the worker. */
    cond_broadcast(&wq->cond);
    return 0;
}

void workqueue_enqueue(workqueue_t *wq, workqueue_job_t *job) {
    (void)workqueue_enqueue_ex(wq, job);
}

int workqueue_cancel_ex(workqueue_t *wq, workqueue_job_t *job) {
    workqueue_cancellation_t cancellation = { .job = job };
    bool queued;
    bool running;

    if(irq_inside_int()) {
        errno = EPERM;
        return -1;
    }
    if(!wq || !job) {
        errno = EINVAL;
        return -1;
    }

    mutex_lock(&wq->lock);
    if(wq->curr_job == job && thd_get_current() == wq->thd) {
        mutex_unlock(&wq->lock);
        errno = EDEADLK;
        return -1;
    }

    while(job_is_cancelling(wq, job))
        (void)cond_wait(&wq->cond, &wq->lock);

    STAILQ_INSERT_TAIL(&wq->cancellations, &cancellation, entry);
    queued = remove_queued_job(wq, job);
    running = wq->curr_job == job;

    while(wq->curr_job == job)
        (void)cond_wait(&wq->cond, &wq->lock);

    /* A callback which raced cancellation may have queued itself immediately
       before the cancellation barrier became visible. */
    queued |= remove_queued_job(wq, job);
    STAILQ_REMOVE(&wq->cancellations, &cancellation,
                  workqueue_cancellation, entry);
    cond_broadcast(&wq->cond);
    mutex_unlock(&wq->lock);

    if(!queued && !running) {
        errno = ENOENT;
        return -1;
    }

    return 0;
}

void workqueue_cancel(workqueue_t *wq, workqueue_job_t *job) {
    (void)workqueue_cancel_ex(wq, job);
}

int workqueue_job_get_info(workqueue_t *wq, workqueue_job_t *job,
                           workqueue_job_info_t *info) {
    if(irq_inside_int()) {
        errno = EPERM;
        return -1;
    }
    if(!wq || !job || !info) {
        errno = EINVAL;
        return -1;
    }

    mutex_lock_scoped(&wq->lock);
    info->queued = job_is_queued(wq, job);
    info->running = wq->curr_job == job;
    info->cancelling = job_is_cancelling(wq, job);
    return 0;
}

void workqueue_kill(workqueue_t *wq) {
    if(irq_inside_int()) {
        errno = EPERM;
        return;
    }
    if(!wq)
        return;

    mutex_lock(&wq->lock);
    if(!wq->quit) {
        wq->quit = true;
        /* Pending jobs remain caller-owned; stopping never executes them. */
        STAILQ_INIT(&wq->jobs);
        cond_broadcast(&wq->cond);
    }

    /* The worker may request its own orderly stop, but it cannot join itself. */
    if(thd_get_current() == wq->thd) {
        mutex_unlock(&wq->lock);
        return;
    }

    while(wq->joining && !wq->joined)
        (void)cond_wait(&wq->cond, &wq->lock);

    if(wq->joined) {
        mutex_unlock(&wq->lock);
        return;
    }

    wq->joining = true;
    mutex_unlock(&wq->lock);

    const int result = thd_join(wq->thd, NULL);

    mutex_lock(&wq->lock);
    if(result == 0) {
        wq->joined = true;
        wq->thd = NULL;
    }
    wq->joining = false;
    cond_broadcast(&wq->cond);
    mutex_unlock(&wq->lock);
    if(result < 0)
        errno = EIO;
}

void workqueue_destroy(workqueue_t *wq) {
    if(irq_inside_int()) {
        errno = EPERM;
        return;
    }
    if(!wq)
        return;
    if(thd_get_current() == wq->thd) {
        errno = EDEADLK;
        return;
    }

    workqueue_kill(wq);
    /* A failed join is not proof that callbacks have stopped using the queue. */
    if(!wq->joined)
        return;
    cond_destroy(&wq->cond);
    mutex_destroy(&wq->lock);
    free(wq);
}

kthread_t *workqueue_get_thread(workqueue_t *wq) {
    if(irq_inside_int()) {
        errno = EPERM;
        return NULL;
    }
    if(!wq) {
        errno = EINVAL;
        return NULL;
    }
    mutex_lock_scoped(&wq->lock);
    return wq->thd;
}
