/* KallistiOS ##version##

   include/kos/rwsem.h
   Copyright (C) 2008, 2010, 2012 Lawrence Sebald

*/

/** \file    kos/rwsem.h
    \brief   Definition for a reader/writer semaphore.
    \ingroup kthreads

    This file defines a concept of reader/writer semaphores. Basically, this
    type of lock allows an unlimited number of "readers" to acquire the lock at
    a time, but only one "writer" (and only if no readers hold the lock).
    Readers, by definition, should not change any global data (since they are
    defined to only be reading), and since this is the case it is safe to allow
    multiple readers to access global data that is shared amongst threads.
    Writers on the other hand require exclusive access since they will be
    changing global data in the critical section, and they cannot share with
    a reader either (since the reader might attempt to read while the writer is
    changing data).

    \author Lawrence Sebald
*/

#ifndef __KOS_RWSEM_H
#define __KOS_RWSEM_H

#include <kos/cdefs.h>

__BEGIN_DECLS

#include <stddef.h>
#include <kos/thread.h>

/** \brief  Reader/writer semaphore structure.

    All members of this structure should be considered to be private, it is not
    safe to change anything in here yourself.

    \headerfile kos/rwsem.h
*/
typedef struct rw_semaphore {
    /** \brief  The number of readers that are currently holding the lock. */
    int read_count;

    /** \brief  The thread holding the write lock. */
    kthread_t *write_lock;

    /** \brief  Space for one reader who's trying to upgrade to a writer. */
    kthread_t *reader_waiting;
} rw_semaphore_t;

/** \brief  Initializer for a transient reader/writer semaphore */
#define RWSEM_INITIALIZER   { 0, NULL, NULL }

/** \brief  Initialize a reader/writer semaphore.

    This function initializes a new reader/writer semaphore for use.

    \retval 0       On success (no error conditions currently defined).
*/
int rwsem_init(rw_semaphore_t *s) __nonnull_all;

/** \brief  Destroy a reader/writer semaphore.

    This function cleans up a reader/writer semaphore. It is an error to attempt
    to destroy a r/w semaphore that is locked either for reading or writing.

    \param  s       The r/w semaphore to destroy.
    \retval 0       On success.
    \retval -1      On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EBUSY - the semaphore is still locked
*/
int rwsem_destroy(rw_semaphore_t *s) __nonnull_all;

/** \brief  Lock a reader/writer semaphore for reading (with a timeout).

    This function attempts to lock the r/w semaphore for reading. If the
    semaphore is locked for writing, this function will block until it is
    possible to obtain the lock for reading or the timeout expires. This
    function is <b>NOT</b> safe to call inside of an interrupt.

    \param  s       The r/w semaphore to lock.
    \param  timeout The maximum time to wait (in milliseconds).
    \retval 0       On success
    \retval -1      On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EPERM - called inside an interrupt \n
    \em     ETIMEDOUT - the timeout expires before the lock can be acquired \n
    \em     EINVAL - the timeout value is invalid \n
    \em     EINVAL - the semaphore is not initialized
*/
int rwsem_read_lock_timed(rw_semaphore_t *s, int timeout) __nonnull_all;

/** \brief  Lock a reader/writer semaphore for reading.

    This function attempts to lock the r/w semaphore for reading. If the
    semaphore is locked for writing, this function will block until it is
    possible to obtain the lock for reading. This function is <b>NOT</b> safe to
    call inside of an interrupt; use rwsem_read_lock_irqsafe instead.

    \param  s       The r/w semaphore to lock.
    \retval 0       On success
    \retval -1      On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EPERM - called inside an interrupt \n
    \em     EINVAL - the semaphore is not initialized
*/
int rwsem_read_lock(rw_semaphore_t *s) __nonnull_all;

/** \brief  Lock a reader/writer semaphore for reading.

    This function attempts to lock the r/w semaphore for reading. If the
    semaphore is locked for writing, this function will block until it is
    possible to obtain the lock for reading.
    If called within an interrupt context, and the semaphore is already locked,
    this function will return an error.

    \param  s       The r/w semaphore to lock.
    \retval 0       On success
    \retval -1      On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EINVAL - the semaphore is not initialized \n
    \em     EWOULDBLOCK - called inside an interrupt and the semaphore was
                          already locked
*/
int rwsem_read_lock_irqsafe(rw_semaphore_t *s) __nonnull_all;

/** \brief  Lock a reader/writer semaphore for writing (with a timeout).

    This function attempts to lock the r/w semaphore for writing. If the
    semaphore is locked for reading or writing, this function will block until
    it is possible to obtain the lock for writing or the timeout expires. This
    function is <b>NOT</b> safe to call inside of an interrupt.

    \param  s       The r/w semaphore to lock.
    \param  timeout The maximum time to wait (in milliseconds).
    \retval 0       On success.
    \retval -1      On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EPERM - called inside an interrupt \n
    \em     ETIMEDOUT - the timeout expires before the lock can be acquired \n
    \em     EINVAL - the timeout value is invalid \n
    \em     EINVAL - the semaphore is not initialized
*/
int rwsem_write_lock_timed(rw_semaphore_t *s, int timeout) __nonnull_all;

/** \brief  Lock a reader/writer semaphore for writing.

    This function attempts to lock the r/w semaphore for writing. If the
    semaphore is locked for reading or writing, this function will block until
    it is possible to obtain the lock for writing. This function is <b>NOT</b>
    safe to call inside of an interrupt; use rwsem_write_lock_irqsafe instead.

    \param  s       The r/w semaphore to lock.
    \retval 0       On success.
    \retval -1      On error, errno will be set as appropriate.

    \par    Error conditions:
    \em     EPERM - called inside an interrupt \n
    \em     EINVAL - the semaphore is not initialized
*/
int rwsem_write_lock(rw_semaphore_t *s) __nonnull_all;

/** \brief  Lock a reader/writer semaphore for writing.

    This function attempts to lock the r/w semaphore for writing. If the
    semaphore is locked for reading or writing, this function will block until
    it is possible to obtain the lock for writing.
    If called within an interrupt context, and the semaphore is already locked,
    this function will return an error.

    \param  s       The r/w semaphore to lock.
    \retval 0       On success.
    \retval -1      On error, errno will be set as appropriate.

    \par    Error conditions:
    \em     EINVAL - the semaphore is not initialized \n
    \em     EWOULDBLOCK - called inside an interrupt and the semaphore was
                          already locked
*/
int rwsem_write_lock_irqsafe(rw_semaphore_t *s) __nonnull_all;

/** \brief  Unlock a reader/writer semaphore from a read lock.

    This function releases one instance of the read lock on the r/w semaphore.

    \param  s       The r/w semaphore to release the read lock on.
    \retval 0       On success.
    \retval -1      On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EPERM - the read lock is not currently held \n
    \em     EINVAL - the semaphore is not initialized
*/
int rwsem_read_unlock(rw_semaphore_t *s) __nonnull_all;

/** \brief  Unlock a reader/writer semaphore from a write lock.

    This function releases one instance of the write lock on the r/w semaphore.

    \param  s       The r/w semaphore to release the write lock on.
    \retval 0       On success.
    \retval -1      On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EPERM - the write lock is not currently held by the calling
                    thread \n
    \em     EINVAL - the semaphore is not initialized
*/
int rwsem_write_unlock(rw_semaphore_t *s) __nonnull_all;

/** \brief  Unlock a reader/writer semaphore.

    This function releases the lock held by the current thread on the specified
    reader/writer semaphore. This function will automatically determine which
    lock is held by the calling thread and release it as appropriate.

    This function is <b>NOT</b> safe to call (in general) if you do not hold the
    lock!

    \param  s       The r/w semaphore to release the lock on.
    \retval 0       On success.
    \retval -1      On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EPERM - the lock is not currently held by the calling thread \n
    \em     EINVAL - the semaphore is not initialized
*/
int rwsem_unlock(rw_semaphore_t *s) __nonnull_all;

/** \brief  Attempt to lock a reader/writer semaphore for reading.

    This function attempts to lock the r/w semaphore for reading. If for any
    reason rwsem_read_lock would normally block, this function will return an
    error. This function is safe to call inside an interrupt.

    \param  s       The r/w semaphore to attempt to lock.
    \retval 0       On success.
    \retval -1      On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EWOULDBLOCK - a call to rwsem_read_lock would block \n
    \em     EINVAL - the semaphore is not initialized
*/
int rwsem_read_trylock(rw_semaphore_t *s) __nonnull_all;

/** \brief  Attempt to lock a reader/writer semaphore for writing.

    This function attempts to lock the r/w semaphore for writing. If for any
    reason rwsem_write_lock would normally block, this function will return an
    error. This function is safe to call inside an interrupt.

    \param  s       The r/w semaphore to attempt to lock.
    \retval 0       On success.
    \retval -1      On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EWOULDBLOCK - a call to rwsem_write_lock would block \n
    \em     EINVAL - the semaphore is not initialized
*/
int rwsem_write_trylock(rw_semaphore_t *s) __nonnull_all;

/** \brief  Upgrade a thread from reader status to writer status (with a
            timeout).

    This function will upgrade the lock on the calling thread from a reader
    state to a writer state. If it cannot do this at the moment, it will block
    until it is possible. This function is <b>NOT</b> safe to call inside an
    interrupt.

    You can only have one reader waiting to upgrade at a time, otherwise the
    state would potentially become corrupted between when this is called and
    when you get the lock. If you get -1 back from this, you must not assume
    that you can write safely! On error, the calling thread will still hold a
    read lock.

    \param  s       The r/w semaphore to upgrade.
    \param  timeout The maximum time to wait (in milliseconds).
    \retval 0       On success.
    \retval -1      On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EPERM - called inside an interrupt \n
    \em     EINVAL - the semaphore is not initialized \n
    \em     EINVAL - the timeout value is invalid \n
    \em     EBUSY - another reader has already requested an upgrade \n
    \em     ETIMEDOUT - the timeout expired before the write lock could be
                        acquired
*/
int rwsem_read_upgrade_timed(rw_semaphore_t *s, int timeout) __nonnull_all;

/** \brief  Upgrade a thread from reader status to writer status.

    This function will upgrade the lock on the calling thread from a reader
    state to a writer state. If it cannot do this at the moment, it will block
    until it is possible. This function is <b>NOT</b> safe to call inside an
    interrupt.

    You can only have one reader waiting to upgrade at a time, otherwise the
    state would potentially become corrupted between when this is called and
    when you get the lock. If you get -1 back from this, you must not assume
    that you can write safely! On error, the calling thread will still hold a
    read lock.

    \param  s       The r/w semaphore to upgrade.
    \retval 0       On success.
    \retval -1      On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EPERM - called inside an interrupt \n
    \em     EINVAL - the semaphore is not initialized \n
    \em     EBUSY - another reader has already requested an upgrade
*/
int rwsem_read_upgrade(rw_semaphore_t *s) __nonnull_all;

/** \brief  Attempt to upgrade a thread from reader status to writer status.

    This function will attempt to upgrade the lock on the calling thread to
    writer status. If for any reason rwsem_read_upgrade would block, this
    function will return an error. This function is safe to call inside an
    interrupt. Note that on error, the read lock is still held!

    \param  s       The r/w semaphore to upgrade.
    \retval 0       On success.
    \retval -1      On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EWOULDBLOCK - a call to rwsem_read_upgrade would block \n
    \em     EBUSY - another reader has already requested an upgrade \n
    \em     EINVAL - the semaphore is not initialized
*/
int rwsem_read_tryupgrade(rw_semaphore_t *s) __nonnull_all;

/** \brief  Read the reader count on the reader/writer semaphore.

    This function is not a safe way to see if the lock will be locked by any
    readers when you get around to locking it, so do not use it in this way.

    \param  s       The r/w semaphore to count the readers on.
    \return The number of readers holding the r/w semaphore.
*/
int rwsem_read_count(rw_semaphore_t *s) __nonnull_all;

/** \brief  Read the state of the writer lock on the reader/writer semaphore.

    This function is not a safe way to see if the lock will be locked by a
    writer by the time you get around to doing something with it, so don't try
    to use it for that purpose.

    \param  s       The r/w semaphore to check the writer status on.
    \return The status of the writer lock of the r/w semaphore.
*/
int rwsem_write_locked(rw_semaphore_t *s) __nonnull_all;

__END_DECLS

#endif /* __KOS_RWSEM_H */
