/* KallistiOS ##version##

   include/kos/oneshot_timer.h
   Copyright (C) 2024 Paul Cercueil
*/

/** \file    kos/oneshot_timer.h
    \brief   One-shot timer support.
    \ingroup kthreads

    This file contains the one-shot timer API. A one-shot timer is a timer that
    will trigger an action (through a pre-registered callback) after a timeout
    expires.

    \author Paul Cercueil

    \see    kos/thread.h
    \see    kos/worker_thread.h
*/

#ifndef __KOS_ONESHOT_TIMER_H
#define __KOS_ONESHOT_TIMER_H

#include <kos/cdefs.h>
__BEGIN_DECLS

struct oneshot_timer;

/** \struct  oneshot_timer_t
    \brief   Opaque structure describing one one-shot timer.
*/
typedef struct oneshot_timer oneshot_timer_t;

/** \brief       Create a new one-shot timer.
    \relatesalso oneshot_timer_t

    This function will create a one-shot timer using the specified callback,
    programmed to expire after the given timeout. The timer will be stopped
    by default and still need to be started using oneshot_timer_start().

    \param  cb              The function to call in case the one-shot timer
                            expires.
    \param  data            A parameter to pass to the function called.
    \param  timeout_ms      The timeout value for the one-shot timer, in
                            milliseconds.

    \return                 The new one-shot timer on success, NULL on failure.

    \sa oneshot_timer_destroy, oneshot_timer_start
*/
oneshot_timer_t *oneshot_timer_create(void (*cb)(void *), void *data,
                                      unsigned int timeout_ms);

/** \brief       Stop and destroy a one-shot timer.
    \relatesalso oneshot_timer_t

    This function will stop the one-shot timer and free its memory.

    \param  timer           A pointer to the one-shot timer.

    \sa oneshot_timer_create
*/
void oneshot_timer_destroy(oneshot_timer_t *timer);

/** \brief       Re-configure a one-shot timer.
    \relatesalso oneshot_timer_t

    This function can be used to change the registered callback or the timeout.
    Using it on a running timer is unsupported.

    \param  timer           A pointer to the one-shot timer.
    \param  cb              The function to call in case the one-shot timer
                            expires.
    \param  data            A parameter to pass to the function called.
    \param  timeout_ms      The timeout value for the one-shot timer, in
                            milliseconds.

    \sa oneshot_timer_stop, oneshot_timer_start
*/
void oneshot_timer_setup(oneshot_timer_t *timer, void (*cb)(void *),
                         void *data, unsigned int timeout_ms);

/** \brief       Start a one-shot timer.
    \relatesalso oneshot_timer_t

    This function will start the one-shot timer. If not stopped until the
    timeout value expires, the one-shot timer will call the registered
    callback function.

    \param  timer           A pointer to the one-shot timer.

    \sa oneshot_timer_stop, oneshot_timer_reset
*/
void oneshot_timer_start(oneshot_timer_t *timer);

/** \brief       Stop a one-shot timer.
    \relatesalso oneshot_timer_t

    This function will stop the one-shot timer. If it already expired, this
    function does nothing. If it did not expire yet, the one-shot timer is
    stopped and the registered callback won't be called.

    \param  timer           A pointer to the one-shot timer.

    \sa oneshot_timer_start, oneshot_timer_reset
*/
void oneshot_timer_stop(oneshot_timer_t *timer);

/** \brief       Reset a one-shot timer.
    \relatesalso oneshot_timer_t

    A convenience function to reset the one-shot timer by stopping it then
    starting it again.

    \param  timer           A pointer to the one-shot timer.

    \sa oneshot_timer_start, oneshot_timer_stop
*/
static inline void oneshot_timer_reset(oneshot_timer_t *timer) {
    oneshot_timer_stop(timer);
    oneshot_timer_start(timer);
}

__END_DECLS

#endif /* __KOS_ONESHOT_TIMER_H */
