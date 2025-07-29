/* KallistiOS ##version##

   worker.c
   Copyright (C) 2024 Paul Cercueil
*/

#include <arch/irq.h>
#include <assert.h>
#include <kos/genwait.h>
#include <kos/thread.h>
#include <kos/worker_thread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/queue.h>

struct kthread_worker {
    kthread_t *thd;
    void (*routine)(void *);
    void *data;
    bool pending;
    bool quit;
    STAILQ_HEAD(kthread_jobs, kthread_job) jobs;
};

static void *thd_worker_thread(void *d) {
    kthread_worker_t *worker = d;
    uint32_t flags;

    for(;;) {
        flags = irq_disable();

        if((!worker->pending) && (!worker->quit))
            genwait_wait(worker, worker->thd->label, 0, NULL);

        irq_restore(flags);

        if(worker->quit)
            break;

        worker->pending = false;
        worker->routine(worker->data);
    }

    free(worker);
    return NULL;
}

kthread_worker_t *thd_worker_create_ex(const kthread_attr_t *attr,
                                       void (*routine)(void *), void *data) {
    kthread_worker_t *worker;
    uint32_t flags;

    assert(routine != NULL);

    worker = malloc(sizeof(*worker));
    if(!worker)
        return NULL;

    worker->data = data;
    worker->routine = routine;
    worker->pending = false;
    worker->quit = false;
    STAILQ_INIT(&worker->jobs);

    flags = irq_disable();

    worker->thd = thd_create_ex(attr, thd_worker_thread, worker);
    if(!worker->thd) {
        irq_restore(flags);
        free(worker);
        return NULL;
    }

    irq_restore(flags);

    return worker;
}

void thd_worker_wakeup(kthread_worker_t *worker) {
    assert(worker != NULL);

    irq_disable_scoped();

    worker->pending = true;
    genwait_wake_one(worker);
}

void thd_worker_destroy(kthread_worker_t *worker) {
    uint32_t flags;
    kthread_t *thd;

    assert(worker != NULL);

    flags = irq_disable();

    worker->quit = true;
    genwait_wake_one(worker);
    thd = worker->thd;

    irq_restore(flags);

    thd_join(thd, NULL);
}

kthread_t *thd_worker_get_thread(kthread_worker_t *worker) {
    return worker->thd;
}

void thd_worker_add_job(kthread_worker_t *worker, kthread_job_t *job) {
    irq_disable_scoped();

    STAILQ_INSERT_TAIL(&worker->jobs, job, entry);
}

kthread_job_t *thd_worker_dequeue_job(kthread_worker_t *worker) {
    kthread_job_t *job;

    irq_disable_scoped();

    job = STAILQ_FIRST(&worker->jobs);
    STAILQ_REMOVE_HEAD(&worker->jobs, entry);

    return job;
}
