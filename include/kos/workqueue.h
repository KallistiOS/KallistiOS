/* KallistiOS ##version##

   include/kos/workqueue.h
   Copyright (C) 2026 Paul Cercueil
*/

/** \file    kos/workqueue.h
    \brief   Threaded work queue support.
    \ingroup kthreads

    This file contains the API to create and manage work queues.

    A work queue is a thread that will execute tasks (aka. jobs) that are
    enqueued by client code, at a predeterminated moment in time. Multiple
    jobs can be enqueued. Once a job is executed, it is removed from the
    execution queue.

    \author Paul Cercueil

    \see    kos/thread.h
    \see    kos/worker_thread.h
*/

#ifndef __KOS_WORKQUEUE_H
#define __KOS_WORKQUEUE_H

#include <kos/cdefs.h>

__BEGIN_DECLS

#include <kos/thread.h>
#include <stdint.h>
#include <sys/queue.h>

struct workqueue;

/** \struct  workqueue_t
    \brief   Opaque structure describing one work queue.
*/
typedef struct workqueue workqueue_t;

/** \struct  workqueue_job_t
    \brief   Structure describing a job for the work queue.
*/
typedef struct workqueue_job {
    /** \brief  Routine to call. */
    void (*cb)(workqueue_t *queue, struct workqueue_job *job);

    /** \brief  Time at which the job will be processed.
                If set to 0, the job will be set to execute immediately. */
    uint64_t time_ms;

    /** \brief  List handle. No need to set manually. */
    STAILQ_ENTRY(workqueue_job) entry;
} workqueue_job_t;

/** \brief       Create a new work queue.
    \relatesalso workqueue_t

    This function will create a new work queue.

    \return                 The new work queue on success, NULL on failure.

    \sa workqueue_destroy
*/
workqueue_t *workqueue_create(void);

/** \brief       Destroy a work queue.
    \relatesalso workqueue_t

    This function will destroy a work queue and free up any allocated memory.

    \param  wq              A pointer to the work queue

    \sa workqueue_create
*/
void workqueue_destroy(workqueue_t *wq);

/** \brief       Stop a work queue from running.
    \relatesalso workqueue_t

    This function can optionally be called before destroying a work queue.
    The work queue then stops processing previously or newly enqueued jobs.

    \param  wq              A pointer to the work queue

    \sa workqueue_destroy
*/
void workqueue_kill(workqueue_t *wq);

/** \brief       Enqueue a job to a work queue.
    \relatesalso workqueue_t

    This function will enqueue a job to the given work queue. The job's struct
    must have been initialized properly.

    \param  wq              A pointer to the work queue
    \param  job             A pointer to the job to enqueue

    \sa workqueue_create
*/
void workqueue_enqueue(workqueue_t *wq, workqueue_job_t *job);

/** \brief       Cancel a job and remove it from the work queue.
    \relatesalso workqueue_t

    This function can be used when a job should be removed from a work queue
    before the job is set to be executed (note that jobs are automatically
    removed from the work queue right before their execution).

    \param  wq              A pointer to the work queue
    \param  job             A pointer to the job to cancel

    \sa workqueue_create
*/
void workqueue_cancel(workqueue_t *wq, workqueue_job_t *job);

/** \brief       Get a handle to the underlying thread.
    \relatesalso workqueue_t

    \param  wq              The workqueue whose thread should be returned.

    \return                 A handle to the underlying thread.
*/
kthread_t *workqueue_get_thread(workqueue_t *wq);

__END_DECLS

#endif /* __KOS_WORKQUEUE_H */
