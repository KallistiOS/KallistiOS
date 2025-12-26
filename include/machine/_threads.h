/* KallistiOS ##version##

   machine/_threads.h
   Copyright (C) 2014 Lawrence Sebald
*/

/** \file   machine/_threads.h
    \brief  C11 Threading API.
    \ingroup threading_c11

    This file contains the platform-specific definitions needed for using 
    C11 threads. The C11 standard defines a number of threading-related
    primitives, which we wrap neatly around KOS' built-in threading support here.

    If you compile your code with a strict standard set (you use a -std= flag
    with GCC that doesn't start with gnu), you must use -std=c11 to use this
    functionality. If you don't pass a -std= flag to GCC, then you're probably
    fine.

    \author Lawrence Sebald
*/

#ifndef __MACHINE_THREADS_H
#define __MACHINE_THREADS_H

#if !defined(__STRICT_ANSI__) || (__STDC_VERSION__ >= 201112L)

#include <sys/cdefs.h>
#include <time.h>

/* Bring in all the threading-related stuff we'll need. */
#include <kos/thread.h>
#include <kos/once.h>
#include <kos/mutex.h>
#include <kos/cond.h>
#include <kos/tls.h>

__BEGIN_DECLS

/** \defgroup threading_c11     C11
    \brief                      C11 Threading APIs
    \ingroup                    threading
    
    @{
*/

/** \brief  Object type backing call_once.

    This object type holds a flag that is used by the call_once function to call
    a function one time. It should always be initialized with the ONCE_FLAG_INIT
    macro.

    \headerfile machine/_threads.h
*/
typedef kthread_once_t once_flag;

/** \brief  Macro to initialize a once_flag object. */
#define ONCE_FLAG_INIT KTHREAD_ONCE_INIT

/** \brief  C11 mutual exclusion lock type.

    This type holds an identifier for a mutual exclusion (mutex) lock to be used
    with C11 threading support.

    \headerfile machine/_threads.h
*/
typedef mutex_t mtx_t;

/** \brief  C11 condition variable type.

    This type holds an identifier for a condition variable object that is to be
    used with C11 threading support.

    \headerfile machine/_threads.h
*/
typedef condvar_t cnd_t;

/** \brief  C11 thread identifier type.

    This type holds an identifier for a C11 thread.

    \headerfile machine/_threads.h
*/
typedef kthread_t *thrd_t;


/** \brief  Maximum number of iterations over TSS destructors.

    This macro defines the maximum number of iterations that will be performed
    over the destructors for thread-specific storage objects when a thread
    terminates.

    \headerfile machine/_threads.h
*/
#define TSS_DTOR_ITERATIONS     1

/** \brief  C11 thread-specific storage type.

    This type holds a thread-specific storage identifier, which allows a value
    to be associated with it for each and every thread running.

    \headerfile machine/_threads.h
*/
typedef kthread_key_t tss_t;

/** @} */

__END_DECLS

#endif /* !defined(__STRICT_ANSI__) || (__STDC_VERSION__ >= 201112L) */

#endif /* !__MACHINE_THREADS_H */
