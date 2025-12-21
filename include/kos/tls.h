/* KallistiOS ##version##

   include/kos/tls.h
   Copyright (C) 2009, 2010 Lawrence Sebald

*/

/** \file   kos/tls.h
    \brief  Thread-local storage support.
    \ingroup kthreads

    This file contains the definitions used to support key/value pairs of
    thread-local storage in KOS.

    \author Lawrence Sebald
*/

#ifndef __KOS_TLS_H
#define __KOS_TLS_H

#include <kos/cdefs.h>

__BEGIN_DECLS

#include <sys/queue.h>

/** \brief  Thread-local storage key type. */
typedef int kthread_key_t;

/** \brief  Thread-local storage key-value pair.

    This is the structure that is actually used to store the specific value for
    a thread for a single TLS key.

    You will not end up using these directly at all in programs, as they are
    only used internally.
*/
typedef struct kthread_tls_kv {
    /** \brief  List handle -- NOT a function. */
    LIST_ENTRY(kthread_tls_kv) kv_list;

    /** \brief  The key associated with this data. */
    kthread_key_t key;

    /** \brief  The value of the data. */
    void *data;

    /** \brief  Optional destructor for the value (set per key). */
    void (*destructor)(void *);
} kthread_tls_kv_t;

/** \cond */
/* TLS Key-Value pair list type. */
LIST_HEAD(kthread_tls_kv_list, kthread_tls_kv);
/** \endcond */

/** \brief  Create a new thread-local storage key.

    This function creates a new thread-local storage key that shall be visible
    to all threads. Each thread is then responsible for associating any data
    with the key that it deems fit (by default a thread will have no data
    associated with a newly created key).

    \param  key         The key to use.
    \param  destructor  A destructor for use with this key. If it is non-NULL,
                        and a value associated with the key is non-NULL at
                        thread exit, then the destructor will be called with the
                        value as its argument.

    \retval 0       On success.
    \retval -1      On error, sets errno as appropriate.

    \par    Error Conditions:
    \em     EPERM - Called inside an interrupt, destructor is not NULL, and there
                    is already a malloc call or destructor operation in progress.
    \em     ENOMEM - Out of memory.

*/
int kthread_key_create(kthread_key_t *key, void (*destructor)(void *));

/** \brief  Retrieve a value associated with a TLS key.

    This function retrieves the thread-specific data associated with the given
    key.

    \param  key     The key to look up data for.
    \return The data associated with the key, or NULL if the key is not valid or
            no data has been set in the current thread.
*/
void *kthread_getspecific(kthread_key_t key);

/** \brief  Set thread specific data for a key.

    This function sets the thread-specific data associated with the given key.

    \param  key     The key to set data for.
    \param  value   The thread-specific value to use.

    \retval 0       On success.
    \retval -1      On error, sets errno as appropriate.

    \par    Error Conditions:
    \em     EINVAL - The key is not valid.
    \em     EPERM - Called inside an interrupt, the key needs to be created, and there
                    is already a malloc call or destructor operation in progress.
    \em     ENOMEM - Out of memory.
*/
int kthread_setspecific(kthread_key_t key, const void *value);

/** \brief  Delete a TLS key.

    This function deletes a TLS key, removing all threads' values for the given
    key. This function <em>does not</em> cause any destructors to be called.

    \param  key     The key to delete.

    \retval 0       On success.
    \retval -1      On error, sets errno as appropriate.

    \par    Error Conditions:
    \em     EINVAL - The key is not valid.
    \em     EPERM - Called inside an interrupt, and there is already a malloc
                    call or destructor operation in progress.
*/
int kthread_key_delete(kthread_key_t key);

/** \cond */
/* Initialization and shutdown. Once again, internal use only. */
int kthread_tls_init(void);
void kthread_tls_shutdown(void);
/** \endcond */

__END_DECLS

#endif /* __KOS_TLS_H */
