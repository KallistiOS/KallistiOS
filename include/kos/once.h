/* KallistiOS ##version##

   include/kos/once.h
   Copyright (C) 2009, 2010, 2023 Lawrence Sebald

*/

#ifndef __KOS_ONCE_H
#define __KOS_ONCE_H

/** \file   kos/once.h
    \brief  Dynamic package initialization.
    \ingroup kthreads

    This file provides definitions for an object that functions the same way as
    the pthread_once_t function does from the POSIX specification. This object
    type and functionality is generally used to make sure that a given
    initialization function is run once, and only once, no matter how many
    threads attempt to call it.

    \author Lawrence Sebald
*/

#include <kos/cdefs.h>

__BEGIN_DECLS

/** \brief  Object type backing kthread_once.

    This object type should always be initialized with the KTHREAD_ONCE_INIT
    macro.

    \headerfile kos/once.h
*/
typedef volatile int kthread_once_t;

/** \brief  Initializer for a kthread_once_t object. */
#define KTHREAD_ONCE_INIT   0

/** \brief  Run a function once.

    This function, when used with a kthread_once_t object (that should be shared
    amongst all threads) will run the init_routine once, and set the
    once_control to make sure that the function will not be run again (as long
    as all threads attempt to call the init_routine through this function.

    \param  once_control    The kthread_once_t object to run against.
    \param  init_routine    The function to call.
    \retval -1      On failure, and sets errno to one of the following: EPERM if
                    called inside an interrupt or EINVAL if *once_control is not
                    valid or was not initialized with KTHREAD_ONCE_INIT.
    \retval 0       On success. */
int kthread_once(kthread_once_t *once_control, void (*init_routine)(void));

__END_DECLS

#endif /* !__KOS_ONCE_H */
