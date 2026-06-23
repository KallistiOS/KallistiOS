/* KallistiOS ##version##

   select.c
   Copyright (C) 2012 Lawrence Sebald
*/

#include <poll.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>

#include <kos/dbglog.h>

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds,
           struct timeval *timeout) {
    int i, added, j = 0, rv, tmout = -1;
    struct pollfd pollfds[nfds > FD_SETSIZE ? 1 : nfds];
    fd_set tmp;

    if(nfds > FD_SETSIZE) {
        errno = EINVAL;
        return -1;
    }

    FD_ZERO(&tmp);

    if(!readfds)
        readfds = &tmp;
    if(!writefds)
        writefds = &tmp;
    if(!errorfds)
        errorfds = &tmp;

    /* Clear all the fds */
    memset(pollfds, 0, sizeof(struct pollfd) * nfds);

    /* Set up the call to poll to do the real work */
    for(i = 0; i < nfds; ++i) {
        added = 0;

        if(FD_ISSET(i, readfds)) {
            pollfds[j].fd = i;
            pollfds[j].events = POLLIN;
            added = 1;
        }

        if(FD_ISSET(i, writefds)) {
            pollfds[j].fd = i;
            pollfds[j].events |= POLLOUT;
            added = 1;
        }

        if(FD_ISSET(i, errorfds)) {
            pollfds[j].fd = i;
            pollfds[j].events |= POLLPRI;
            added = 1;
        }

        if(added)
            ++j;
    }

    if(timeout)
        tmout = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;

    /* Poll for a response */
    if((rv = poll(pollfds, j, tmout)) < 0) {
        return rv;
    }

    rv = 0;
    FD_ZERO(readfds);
    FD_ZERO(writefds);
    FD_ZERO(errorfds);

    /* We've gotten a response, lets see what we have */
    for(i = 0; i < j; ++i) {
        /* Make sure there wasn't an error */
        if(pollfds[i].revents & POLLNVAL) {
            errno = EBADF;
            return -1;
        }

        if(pollfds[i].revents & POLLIN) {
            FD_SET(pollfds[i].fd, readfds);
            ++rv;
        }
        if(pollfds[i].revents & POLLOUT) {
            FD_SET(pollfds[i].fd, writefds);
            ++rv;
        }
        if((pollfds[i].events & POLLPRI) &&
           (pollfds[i].revents & (POLLPRI | POLLERR | POLLHUP))) {
            FD_SET(pollfds[i].fd, errorfds);
            ++rv;
        }
    }

    return rv;
}
