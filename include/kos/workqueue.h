/* KallistiOS ##version##

   include/kos/workqueue.h
   Copyright (C) 2026 Paul Cercueil
   Copyright (C) 2026 Joseph Black
*/

/** \file    kos/workqueue.h
    \brief   Threaded work queue support.
    \ingroup kthreads

    This file contains the API to create and manage work queues.

    A work queue is a thread that will execute tasks (aka. jobs) that are
    enqueued by client code, at a predeterminated moment in time. Multiple
    jobs can be enqueued. Once a job is executed, it is removed from the
    execution queue.

    All operations require thread context. Queue destruction must be serialized
    with every other API caller; concurrent kill calls are supported, concurrent
    destruction is not. Job storage belongs to the caller and may belong to
    only one queue at a time. Do not change or free a pending job. A running
    callback may update and requeue itself once, then must stop modifying its
    queued fields. Cancellation drains that callback, but callers must also
    stop independent producers before releasing the job's storage.

    \author Paul Cercueil
    \author Joseph Black

    \see    kos/thread.h
    \see    kos/worker_thread.h
*/

#ifndef __KOS_WORKQUEUE_H
#define __KOS_WORKQUEUE_H

#include <kos/cdefs.h>

__BEGIN_DECLS

#include <kos/thread.h>
#include <stdbool.h>
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
    /** \brief  Time at which the job will be processed.
                If set to 0, the job will be set to execute immediately. */
    uint64_t time_ms;

    /** \brief  Routine to call. */
    void (*cb)(workqueue_t *queue, struct workqueue_job *job);

    /** \brief  List handle. No need to set manually. */
    STAILQ_ENTRY(workqueue_job) entry;
} workqueue_job_t;

/** \brief Coherent state of one job relative to a work queue. */
typedef struct workqueue_job_info {
    bool queued;       /**< \brief Waiting in the deadline-ordered queue. */
    bool running;      /**< \brief Its callback is currently executing. */
    bool cancelling;   /**< \brief Cancellation is suppressing requeue. */
} workqueue_job_info_t;

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
    It must not be called from a callback running on the queue itself; such a
    call is rejected with `errno` set to `EDEADLK` and leaves the queue alive.
    A failed worker join leaves the queue allocated with `errno` set to `EIO`;
    its owner must resolve the cause and retry. IRQ-context calls are rejected
    with `EPERM`.

    \param  wq              A pointer to the work queue

    \sa workqueue_create
*/
void workqueue_destroy(workqueue_t *wq);

/** \brief       Stop a work queue from running.
    \relatesalso workqueue_t

    This function can optionally be called before destroying a work queue.
    The work queue then stops processing previously or newly enqueued jobs.
    A callback may call this function to request an orderly stop. In that case
    it returns without trying to join its own thread; a later call from another
    thread completes the join.
    Pending jobs are discarded, not executed. A non-worker caller waits for the
    active callback to return. Repeated and concurrent calls join only once.
    On join failure, `errno` is `EIO` and the queue remains stopped and allocated
    for retry. A callback's self-stop does not by itself release job storage.

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

/** \brief Enqueue a job with validation and a status result.

    Unlike workqueue_enqueue(), this reports malformed jobs, duplicate pending
    insertion, a stopped queue, and requeue suppressed by cancellation.
    A running callback may enqueue itself once for periodic operation.

    This function is not interrupt-safe.

    \retval 0  Job enqueued.
    \retval -1 Error, with `EINVAL`, `EBUSY`, `ECANCELED`, or `EPERM`.
*/
int workqueue_enqueue_ex(workqueue_t *wq, workqueue_job_t *job);

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

/** \brief Cancel a job safely and report whether it was present.

    A queued job is removed. If its callback is running, this call suppresses
    callback requeue and waits for execution to return. Calling this from that
    same callback returns `EDEADLK` instead of waiting on itself.

    This function is not interrupt-safe.

    \retval 0  A queued or running instance was cancelled and drained.
    \retval -1 Error, with `ENOENT`, `EDEADLK`, `EINVAL`, or `EPERM`.
*/
int workqueue_cancel_ex(workqueue_t *wq, workqueue_job_t *job);

/** \brief Copy a coherent queued/running/cancelling state snapshot. */
int workqueue_job_get_info(workqueue_t *wq, workqueue_job_t *job,
                           workqueue_job_info_t *info);

/** \brief       Get a handle to the underlying thread.
    \relatesalso workqueue_t

    The result is borrowed and becomes invalid when the queue is joined.
    Do not join, detach, or destroy this thread independently. Serialize use of
    the result against shutdown. Returns NULL after a successful join.

    \param  wq              The workqueue whose thread should be returned.

    \return                 A handle to the underlying thread.
*/
kthread_t *workqueue_get_thread(workqueue_t *wq);

__END_DECLS

#endif /* __KOS_WORKQUEUE_H */
