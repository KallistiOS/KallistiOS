/* KallistiOS ##version##

   kernel/arch/dreamcast/hardware/vblank.c
   Copyright (C)2003 Megan Potter
*/

#include <stdlib.h>
#include <errno.h>
#include <sys/queue.h>

#include <arch/irq.h>
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

/* Our internal IRQ handler */
static void vblank_handler(uint32 src, void *data) {
    struct vblhnd * t;

    (void)data;

    TAILQ_FOREACH(t, &vblhnds, listent) {
        t->handler(src, t->data);
    }
}

int vblank_handler_add(asic_evt_handler hnd, void *data) {
    struct vblhnd * vh;
    int old;

    vh = malloc(sizeof(struct vblhnd));

    if(!vh) {
        errno = ENOMEM;
        return -1;
    }

    /* Disable ints just in case */
    old = irq_disable();

    /* Find a new ID */
    vh->id = vblid_high;
    vblid_high++;

    /* Finish filling the struct */
    vh->handler = hnd;
    vh->data = data;

    /* Add it to the list */
    TAILQ_INSERT_TAIL(&vblhnds, vh, listent);

    /* Restore ints */
    irq_restore(old);

    return vh->id;
}

int vblank_handler_remove(int handle) {
    struct vblhnd * t;
    int old, rv;

    /* Disable ints just in case */
    old = irq_disable();

    /* Look for it */
    rv = -1;
    TAILQ_FOREACH(t, &vblhnds, listent) {
        if(t->id == handle) {
            TAILQ_REMOVE(&vblhnds, t, listent);
            free(t);
            rv = 0;
            break;
        }
    }

    /* Restore ints */
    irq_restore(old);

    return rv;
}

int vblank_init(void) {
    /* Setup our data structures */
    TAILQ_INIT(&vblhnds);
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

    /* Free any allocated handlers */
    c = TAILQ_FIRST(&vblhnds);

    while(c != NULL) {
        n = TAILQ_NEXT(c, listent);
        free(c);
        c = n;
    }

    TAILQ_INIT(&vblhnds);

    return 0;
}


