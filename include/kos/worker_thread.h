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
    corresponding work function.

    \param  thd             The worker thread to wake up.

    \sa thd_worker_create, thd_worker_destroy
*/
void thd_worker_wakeup(kthread_worker_t *thd);

/** \brief       Get a handle to the underlying thread.
    \relatesalso kthread_worker_t

    \param  thd             The worker thread whose handle should be returned.

    \return                 A handle to the underlying thread.
*/
kthread_t *thd_worker_get_thread(kthread_worker_t *thd);

__END_DECLS

#endif /* __KOS_WORKER_THREAD_H */
