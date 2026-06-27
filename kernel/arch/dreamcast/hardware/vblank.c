/* KallistiOS ##version##

   kernel/arch/dreamcast/hardware/vblank.c
   Copyright (C)2003 Megan Potter
*/

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/queue.h>

#include <kos/mutex.h>
#include <dc/vblank.h>

/*
   Functions to multiplex the vblank IRQ out to N client routines.
   This module is necessary because a number of things need to hang
   off the vblank IRQ, and chaining is unreliable.
*/

/* Our list of handlers */
struct vblhnd {
    TAILQ_ENTRY(vblhnd) listent;
    int         id;
    asic_evt_handler    handler;
    void *data;
};
static TAILQ_HEAD(vhlist, vblhnd) vblhnds;
static int vblid_high;

static mutex_t vbl_tailq_mutex = MUTEX_INITIALIZER;

/* Our internal IRQ handler */
static void vblank_handler(uint32_t src, void *data) {
    struct vblhnd *t;

    (void)data;

    TAILQ_FOREACH(t, &vblhnds, listent) {
        t->handler(src, t->data);
    }
}

int vblank_handler_add(asic_evt_handler hnd, void *data) {
    struct vblhnd *vh = malloc(sizeof(struct vblhnd));

    if(!vh) {
        errno = ENOMEM;
        return -1;
    }

    /* Ensure thread safety for tailq access */
    mutex_lock_scoped(&vbl_tailq_mutex);
    /* Disable events until we're done manipulating the tailq */
    asic_evt_disable_scoped(ASIC_EVT_PVR_VBLANK_BEGIN, ASIC_IRQ_DEFAULT);

    /* Find a new ID */
    vh->id = vblid_high;
    vblid_high++;

    /* Finish filling the struct */
    vh->handler = hnd;
    vh->data = data;

    /* Add it to the list */
    TAILQ_INSERT_TAIL(&vblhnds, vh, listent);

    return vh->id;
}

int vblank_handler_remove(int handle) {
    struct vblhnd *t;

    /* Ensure thread safety for tailq access */
    mutex_lock_scoped(&vbl_tailq_mutex);
    /* Disable events until we're done manipulating the tailq */
    asic_evt_disable_scoped(ASIC_EVT_PVR_VBLANK_BEGIN, ASIC_IRQ_DEFAULT);

    /* Look for it */
    TAILQ_FOREACH(t, &vblhnds, listent) {
        if(t->id == handle) {
            TAILQ_REMOVE(&vblhnds, t, listent);
            free(t);
            return 0;
        }
    }
    return -1;
}

int vblank_init(void) {
    /* Setup our data structures */
    TAILQ_INIT(&vblhnds);
    mutex_init(&vbl_tailq_mutex, MUTEX_TYPE_NORMAL);
    vblid_high = 1;

    /* Hook and enable the interrupt */
    asic_evt_set_handler(ASIC_EVT_PVR_VBLANK_BEGIN, vblank_handler, NULL);
    asic_evt_enable(ASIC_EVT_PVR_VBLANK_BEGIN, ASIC_IRQ_DEFAULT);

    return 0;
}

int vblank_shutdown(void) {
    struct vblhnd * c, * n;

    /* Disable and unhook the interrupt */
    asic_evt_disable(ASIC_EVT_PVR_VBLANK_BEGIN, ASIC_IRQ_DEFAULT);
    asic_evt_remove_handler(ASIC_EVT_PVR_VBLANK_BEGIN);

    mutex_lock(&vbl_tailq_mutex);

    /* Free any allocated handlers */
    c = TAILQ_FIRST(&vblhnds);

    while(c != NULL) {
        n = TAILQ_NEXT(c, listent);
        free(c);
        c = n;
    }

    mutex_unlock(&vbl_tailq_mutex);
    mutex_destroy(&vbl_tailq_mutex);

    TAILQ_INIT(&vblhnds);

    return 0;
}


