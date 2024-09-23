/* KallistiOS ##version##

   poll.h
   Copyright (C) 2012 Lawrence Sebald
*/

/** \file    poll.h
    \brief   Definitions for the poll() function.
    \ingroup threading_polling

    This file contains the definitions needed for using the poll() function, as
    directed by the POSIX 2008 standard (aka The Open Group Base Specifications
    Issue 7). Currently the functionality defined herein only works for sockets,
    and that is likely how it will stay for some time.

    The poll() function works quite similarly to the select() function that it
    is quite likely that you'd be more familiar with.

    \author Lawrence Sebald
*/

#ifndef __POLL_H
#define __POLL_H

#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

/** \defgroup threading_polling  Polling
    \brief                       Implementation of POSIX polling.
    \ingroup                     threading_posix
    @{
*/

/** \brief   Type representing a number of file descriptors. 
    \ingroup threading_polling
 */
typedef __uint32_t nfds_t;

/** \brief   Structure representing a single file descriptor used by poll().
    \ingroup threading_polling
    \headerfile poll.h
*/
struct pollfd {
    int fd;                 /**< \brief The file descriptor in question. */
    short events;           /**< \brief Events to poll for on input. */
    short revents;          /**< \brief Events signalled for output. */
};

/** \defgroup poll_events               Events for the poll() function
    \brief                              Masks representing event types for poll()
    \ingroup                            threading_polling

    These are the events that can be set in the events or revents fields of the
    struct pollfd.

    @{
*/
#define POLLRDNORM  (1 << 0)    /**< \brief Normal data may be read */
#define POLLRDBAND  (1 << 1)    /**< \brief Priority data may be read */
#define POLLPRI     (1 << 2)    /**< \brief High-priority data may be read */
#define POLLOUT     (1 << 3)    /**< \brief Normal data may be written */
#define POLLWRNORM  POLLOUT     /**< \brief Normal data may be written */
#define POLLWRBAND  (1 << 4)    /**< \brief Priority data may be written */
#define POLLERR     (1 << 5)    /**< \brief Error has occurred (revents only) */
#define POLLHUP     (1 << 6)    /**< \brief Peer disconnected (revents only) */
#define POLLNVAL    (1 << 7)    /**< \brief Invalid fd (revents only) */

/** \brief  Data other than high-priority data may be read */
#define POLLIN      (POLLRDNORM | POLLRDBAND)
/** @} */

/** \brief   Poll a group of file descriptors for activity.
    \ingroup threading_polling

    This function will poll a group of file descriptors to check for the events
    specified on them. The function shall block for the specified period of time
    (in milliseconds) waiting for an event to occur. The function shall return
    as soon as at least one fd matches the events specified (or one of the error
    conditions), or when timeout expires.

    \param  fds         The file descriptors to check, and what events to look
                        for on each.
    \param  nfds        Number of elements in fds.
    \param  timeout     Maximum amount of time to block, in milliseconds. Pass
                        0 to ensure the function does not block and -1 to block
                        for an "infinite" amount of time, until an event occurs.
    
    \return             -1 on error (sets errno as appropriate), or the number
                        of file descriptors that matched the event flags before
                        the function returns.
    
    \sa     poll_events
*/
int poll(struct pollfd fds[], nfds_t nfds, int timeout);

/** @} */

__END_DECLS

#endif /* !POLL_H */
