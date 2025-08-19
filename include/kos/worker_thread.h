/* KallistiOS ##version##

   include/kos/worker_thread.h
   Copyright (C) 2024 Paul Cercueil
*/

/** \file    kos/worker_thread.h
    \brief   Threaded worker support.
    \ingroup kthreads

    This file contains the threaded worker API. Threaded workers are threads
    that are idle most of the time, until they are notified that there is work
    pending; in which case they will call their associated work function.

    The work function can then process any number of tasks, until it clears out
    all of its tasks or decides that it worked enough; in which case the
    function can return, and will re-start the next time it is notified, or if
    it was notified while it was running.

    An optional API is also present, which provides a FIFO for jobs to be
    processed by the threaded worker. This is useful when jobs have to be
    processed in sequence.

    \author Paul Cercueil

    \see    kos/thread.h
*/

#ifndef __KOS_WORKER_THREAD_H
#define __KOS_WORKER_THREAD_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <kos/thread.h>
#include <sys/queue.h>

struct kthread_worker;

/** \struct  kthread_worker_t
    \brief   Opaque structure describing one worker thread.
*/
typedef struct kthread_worker kthread_worker_t;

/** \brief   Structure describing one job for the worker. */
typedef struct kthread_job {
    /** \brief  List handle. */
    STAILQ_ENTRY(kthread_job) entry;

    /** \brief  User pointer to the work data. */
    void *data;
} kthread_job_t;

/** \brief       Create a new worker thread with the specific set of attributes.
    \relatesalso kthread_worker_t

    This function will create a thread with the specified attributes that will
    call the given routine with the given param pointer when notified.
    The thread will only stop when thd_worker_destroy() is called.

    \param  attr            A set of thread attributes for the created thread.
                            Passing NULL will initialize all attributes to their
                            default values.
    \param  routine         The function to call in the worker thread.
    \param  data            A parameter to pass to the function called.

    \return                 The new worker thread on success, NULL on failure.

    \sa thd_worker_destroy, thd_worker_wakeup
*/
kthread_worker_t *thd_worker_create_ex(const kthread_attr_t *attr,
                                        void (*routine)(void *), void *data);

/** \brief       Create a new worker thread.
    \relatesalso kthread_worker_t

    This function will create a thread with the default attributes that will
    call the given routine with the given param pointer when notified.
    The thread will only stop when thd_worker_destroy() is called.

    \param  routine         The function to call in the worker thread.
    \param  data            A parameter to pass to the function called.

    \return                 The new worker thread on success, NULL on failure.

    \sa thd_worker_destroy, thd_worker_wakeup
*/
static inline kthread_worker_t *
thd_worker_create(void (*routine)(void *), void *data) {
    return thd_worker_create_ex(NULL, routine, data);
}

/** \brief       Stop and destroy a worker thread.
    \relatesalso kthread_worker_t

    This function will stop the worker thread and free its memory.

    \param  thd             The worker thread to destroy.

    \sa thd_worker_create, thd_worker_wakeup
*/
void thd_worker_destroy(kthread_worker_t *thd);

/** \brief       Wake up a worker thread.
    \relatesalso kthread_worker_t

    This function will wake up the worker thread, causing it to call its
    corresponding work function. Usually, this should be called after a new
    job has been added with thd_worker_add_job().

    \param  thd             The worker thread to wake up.

    \sa thd_worker_create, thd_worker_destroy, thd_worker_add_job
*/
void thd_worker_wakeup(kthread_worker_t *thd);

/** \brief       Get a handle to the underlying thread.
    \relatesalso kthread_worker_t

    \param  thd             The worker thread whose handle should be returned.

    \return                 A handle to the underlying thread.
*/
kthread_t *thd_worker_get_thread(kthread_worker_t *thd);

/** \brief       Add a new job to the worker thread.
    \relatesalso kthread_worker_t

    This function will append the job to the worker thread's to-do queue.
    Note that it is the responsability of the work function (the one passed to
    thd_worker_create()) to dequeue and process the jobs with
    thd_worker_dequeue_job(). Also, this function won't automatically notify the
    worker thread - you still need to call thd_worker_wakeup().

    \param  thd             The worker thread to add a job to.
    \param  job             The new job to give to the worker thread.
*/
void thd_worker_add_job(kthread_worker_t *thd, kthread_job_t *job);

/** \brief       Dequeue one job from the worker thread's to-do queue.
    \relatesalso kthread_worker_t

    Use this function to dequeue one job from the worker thread, that has been
    previously queued using thd_worker_add_job(). This function is typically
    used inside the work function registered with thd_worker_create().

    \param  worker          The worker thread to add a job to.

    \return                 A new job to process, or NULL if there is none.
*/
kthread_job_t *thd_worker_dequeue_job(kthread_worker_t *worker);

__END_DECLS

#endif /* __KOS_WORKER_THREAD_H */
