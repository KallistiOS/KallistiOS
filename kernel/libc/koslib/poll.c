/* KallistiOS ##version##

   poll.c
   Copyright (C) 2012 Lawrence Sebald

*/

#include <poll.h>
#include <errno.h>
#include <sys/queue.h>

#include <arch/irq.h>
#include <kos/fs.h>
#include <kos/mutex.h>
#include <kos/cond.h>

struct poll_int {
    LIST_ENTRY(poll_int) entry;
    struct pollfd *fds;
    nfds_t nfds;
    int nmatched;
    condvar_t cv;
};

LIST_HEAD(polllist, poll_int) poll_list;

static mutex_t mutex = MUTEX_INITIALIZER;

void __poll_event_trigger(int fd, short event) {
    struct poll_int *i;
    nfds_t j;
    int gotone = 0;
    short mask;

    if(mutex_lock_irqsafe(&mutex))
        /* XXXX: Uhh... this is bad... */
        return;

    /* Look through the list of poll fds for any that match */
    LIST_FOREACH(i, &poll_list, entry) {
        for(j = 0; j < i->nfds; ++j) {
            if(i->fds[j].fd == fd) {
                mask = i->fds[j].events | POLLERR | POLLHUP | POLLNVAL;
                if(event & mask) {
                    i->fds[j].revents |= event & mask;
                    ++i->nmatched;
                    gotone = 1;
                }
            }
        }

        /* If we got any events, signal the waiting thread to wake it up. */
        if(gotone) {
            cond_signal(&i->cv);
            gotone = 0;
        }
    }

    mutex_unlock(&mutex);
}

int poll(struct pollfd fds[], nfds_t nfds, int timeout) {
    struct poll_int p = { { 0 }, fds, nfds, 0, COND_INITIALIZER };
    int tmp;
    nfds_t i;
    vfs_handler_t *hndl;
    void *hnd;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    /* Check if any of the fds already match */
    for(i = 0; i < nfds; ++i) {
        hndl = fs_get_handler(fds[i].fd);
        hnd = fs_get_handle(fds[i].fd);
        fds[i].revents = 0;

        /* If we didn't get one of these, then assume its a bad fd. */
        if(!hndl || !hnd) {
            fds[i].revents = POLLNVAL;
            ++p.nmatched;
            continue;
        }

        if(!hndl->poll) {
            /* Assume its a regular file if there's no poll method in the
               handler. */
            if(fds[i].events & (POLLRDNORM | POLLWRNORM)) {
                fds[i].revents |= (POLLRDNORM | POLLWRNORM) & fds[i].events;
                ++p.nmatched;
            }
        }
        else {
            if((fds[i].revents = hndl->poll(hnd, fds[i].events))) {
                ++p.nmatched;
            }
        }
    }

    /* If the user specified a 0 timeout, or we've already matched something,
       bail out now. */
    if(p.nmatched || !timeout) {
        mutex_unlock(&mutex);
        return p.nmatched;
    }

    /* We can't actually wait while we're in an interrupt, so if we got this far
       it is an error. */
    if(irq_inside_int()) {
        mutex_unlock(&mutex);
        errno = EPERM;
        return -1;
    }

    /* Map to the value used by cond_wait_timed() */
    if(timeout == -1)
        timeout = 0;

    /* Add this instance to the list */
    LIST_INSERT_HEAD(&poll_list, &p, entry);

    tmp = errno;
    if(cond_wait_timed(&p.cv, &mutex, timeout)) {
        errno = tmp;
        tmp = 0;
        goto out;
    }

    tmp = p.nmatched;

out:
    /* Remove this instance from the list */
    LIST_REMOVE(&p, entry);

    mutex_unlock(&mutex);
    return tmp;
}
