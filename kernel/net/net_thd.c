/* KallistiOS ##version##

   kernel/net/net_thd.c
   Copyright (C) 2009, 2012, 2013 Lawrence Sebald

*/

#include <sys/queue.h>
#include <errno.h>
#include <stdlib.h>

#include <kos/thread.h>
#include <arch/timer.h>
#include "net_thd.h"

struct thd_cb {
    TAILQ_ENTRY(thd_cb) thds;

    int cbid;
    void (*cb)(void *);
    void *data;
    uint64 timeout;
    uint64 nextrun;
};

TAILQ_HEAD(thd_cb_queue, thd_cb);

static struct thd_cb_queue cbs;
static kthread_t *thd;
static int done = 0;
static int cbid_top;

static void *net_thd_thd(void *data) {
    struct thd_cb *cb;
    uint64 now;

    (void)data;

    while(!done) {
        now = timer_ms_gettime64();

        /* Run any callbacks that need to be run now. */
        TAILQ_FOREACH(cb, &cbs, thds) {
            if(now >= cb->nextrun) {
                cb->cb(cb->data);
                cb->nextrun = now + cb->timeout;
            }
        }

        /* Go to sleep til we need to be run again. */
        thd_sleep(50);
    }

    return NULL;
}

int net_thd_add_callback(void (*cb)(void *), void *data, uint64 timeout) {
    struct thd_cb *newcb;

    /* Allocate space for the new callback and set it up. */
    newcb = (struct thd_cb *)malloc(sizeof(struct thd_cb));

    if(!newcb) {
        errno = ENOMEM;
        return -1;
    }

    newcb->cbid = cbid_top++;
    newcb->cb = cb;
    newcb->data = data;
    newcb->timeout = timeout;
    newcb->nextrun = timer_ms_gettime64() + timeout;

    /* Disable interrupts, insert, and re-enable interrupts */
    irq_disable_scoped();

    TAILQ_INSERT_TAIL(&cbs, newcb, thds);

    return newcb->cbid;
}

int net_thd_del_callback(int cbid) {
    struct thd_cb *cb;

    /* Disable interrupts so we can search without fear of anything changing
       underneath us. */
    irq_disable_scoped();

    /* See if we can find the callback requested. */
    TAILQ_FOREACH(cb, &cbs, thds) {
        if(cb->cbid == cbid) {
            TAILQ_REMOVE(&cbs, cb, thds);
            free(cb);
            return 0;
        }
    }

    /* We didn't find it, punt. */
    return -1;
}

int net_thd_is_current(void) {
    return thd_current == thd;
}

void net_thd_kill(void) {
    /* Do things gracefully, if we can... Otherwise, punt. */
    done = 1;

    if(!irq_inside_int()) {
        thd_join(thd, NULL);
    }
    else {
        thd_destroy(thd);
    }

    thd = NULL;
}

int net_thd_init(void) {
    TAILQ_INIT(&cbs);
    done = 0;
    cbid_top = 1;

    thd = thd_create(0, &net_thd_thd, NULL);

    return 0;
}

void net_thd_shutdown(void) {
    struct thd_cb *c, *n;

    /* Kill the thread. */
    if(thd) {
        net_thd_kill();
    }

    /* Free any handlers that we have laying around */
    c = TAILQ_FIRST(&cbs);

    while(c) {
        n = TAILQ_NEXT(c, thds);
        free(c);
        c = n;
    }

    TAILQ_INIT(&cbs);
}
