/* KallistiOS ##version##

   kernel/arch/dreamcast/hardware/vblank.c
   Copyright (C)2003 Megan Potter
*/

#include <stdint.h>
#include <stdlib.h>
#include <sys/queue.h>

#include <kos/mutex.h>
#include <dc/asic.h>
#include <dc/vblank.h>

/*
   Functions to multiplex the vblank IRQ out to N client routines.
   This module is necessary because a number of things need to hang
   off the vblank IRQ, and chaining is unreliable.
*/

/* Our list of handlers */
struct vblhnd {
    TAILQ_ENTRY(vblhnd) listent;
    asic_evt_handler    handler;
    void                *data;
};
static TAILQ_HEAD(vhlist, vblhnd) vblhnds;

static mutex_t vbl_tailq_mutex = MUTEX_INITIALIZER;

/* Our internal IRQ handler */
static void vblank_handler(uint32_t src, void *data) {
    vblhnd_t *t;

    (void)data;

    TAILQ_FOREACH(t, &vblhnds, listent) {
        t->handler(src, t->data);
    }
}

vblhnd_t *vblank_handler_add(asic_evt_handler hnd, void *data) {
    vblhnd_t *vh = malloc(sizeof(vblhnd_t));

    if(!vh) return NULL;

    /* Ensure thread safety for tailq access */
    mutex_lock_scoped(&vbl_tailq_mutex);
    /* Disable events until we're done manipulating the tailq */
    asic_evt_disable_scoped(ASIC_EVT_PVR_VBLANK_BEGIN, ASIC_IRQ_DEFAULT);

    /* Finish filling the struct */
    vh->handler = hnd;
    vh->data = data;

    /* Add it to the list */
    TAILQ_INSERT_TAIL(&vblhnds, vh, listent);

    return vh;
}

void vblank_handler_remove(vblhnd_t *handle) {
    /* Ensure thread safety for tailq access */
    mutex_lock_scoped(&vbl_tailq_mutex);
    /* Disable events until we're done manipulating the tailq */
    asic_evt_disable_scoped(ASIC_EVT_PVR_VBLANK_BEGIN, ASIC_IRQ_DEFAULT);

    TAILQ_REMOVE(&vblhnds, handle, listent);
    free(handle);
}

int vblank_init(void) {
    /* Setup our data structures */
    TAILQ_INIT(&vblhnds);
    mutex_init(&vbl_tailq_mutex, MUTEX_TYPE_NORMAL);

    /* Hook and enable the interrupt */
    asic_evt_set_handler(ASIC_EVT_PVR_VBLANK_BEGIN, vblank_handler, NULL);
    asic_evt_enable(ASIC_EVT_PVR_VBLANK_BEGIN, ASIC_IRQ_DEFAULT);

    return 0;
}

int vblank_shutdown(void) {
    vblhnd_t *c, *n;

    /* Disable and unhook the interrupt */
    asic_evt_disable(ASIC_EVT_PVR_VBLANK_BEGIN, ASIC_IRQ_DEFAULT);
    asic_evt_remove_handler(ASIC_EVT_PVR_VBLANK_BEGIN);

    mutex_lock(&vbl_tailq_mutex);

    /* Free any allocated handlers */
    TAILQ_FOREACH_SAFE(c, &vblhnds, listent, n) {
        TAILQ_REMOVE(&vblhnds, c, listent);
        free(c);
    }

    mutex_unlock(&vbl_tailq_mutex);
    mutex_destroy(&vbl_tailq_mutex);

    TAILQ_INIT(&vblhnds);

    return 0;
}
