/* KallistiOS ##version##

   maple_irq.c
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2015 Lawrence Sebald
   Copyright (C) 2016 Joe Fenton
 */

#include <malloc.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <dc/maple.h>
#include <dc/asic.h>
#include <dc/pvr.h>
#include <kos/dbglog.h>
#include <kos/thread.h>

/*********************************************************************/
/* VBlank IRQ handler */

/* Maple frame used for attach/detach detection. */
static maple_frame_t detect_frame;

/* Fwd declare */
static void vbl_autodet_callback(maple_state_t *state, maple_frame_t *frm);

/* Send a DEVINFO command for the given port/unit */
static bool vbl_send_devinfo(maple_frame_t *frame, int p, int u) {
    /* Reserve access; if we don't get it, forget about it */
    if(maple_frame_lock(frame) < 0)
        return false;

    /* Setup our autodetect frame to probe at a new device */
    maple_frame_init(frame);
    frame->cmd = MAPLE_COMMAND_DEVINFO;
    frame->dst_port = p;
    frame->dst_unit = u;
    frame->callback = vbl_autodet_callback;
    maple_queue_frame(frame);

    return true;
}

/* Do a potential disconnect on the named device (check to make sure it
   was connected first) */
static void vbl_chk_disconnect(maple_state_t *state, int p, int u) {
    (void)state;

    if(maple_dev_valid(p, u)) {
        if(__is_defined(MAPLE_IRQ_DEBUG)) {
            dbglog(DBG_KDEBUG, "maple: detach on device %c%c\n",
                   'A' + p, '0' + u);
        }

        if(maple_driver_detach(p, u) >= 0) {
            assert(!maple_dev_valid(p, u));
        }
    }
}

static void vbl_chk_next_subdev(maple_state_t *state, maple_frame_t *frm, int p) {
    maple_device_t *dev = maple_enum_dev(p, 0);
    int u;

    if(dev && dev->probe_mask) {
        u = __builtin_ffs(dev->probe_mask);
        dev->probe_mask &= ~(1 << (u - 1));

        vbl_send_devinfo(frm, p, u);
    } else {
        /* Nothing else to probe on this port */
        state->scan_ready_mask |= 1 << p;
    }
}

static void vbl_dev_probed(int p, int u) {
    maple_device_t *dev = maple_enum_dev(p, 0);

    if(dev)
        dev->dev_mask |= 1 << (u - 1);
}

/* Check the sub-devices for a top-level port */
static void vbl_chk_subdevs(maple_state_t *state, int p, uint8 newmask) {
    maple_device_t *dev = maple_enum_dev(p, 0);
    unsigned int u;

    newmask &= (1 << (MAPLE_UNIT_COUNT - 1)) - 1;

    /* Disconnect any device that disappeared */
    for(u = 1; u < MAPLE_UNIT_COUNT; u++) {
        if(dev->dev_mask & ~newmask & (1 << (u - 1))) {
            vbl_chk_disconnect(state, p, u);
        }
    }

    dev->dev_mask &= newmask;
    dev->probe_mask = newmask & ~dev->dev_mask;
}

/* Handles autodetection of hotswapping; basically every periodic
   interrupt, we choose a new port/unit pair and probe it for
   device info. If the same thing is plugged in there, we assume
   that nothing has changed. Otherwise, some device (un/re)binding has
   to occur.

   Note that we could actually set this up as a maple device driver,
   but that might complicate things if we're swapping device structures
   around in the middle of a list traversal, so we do it here as a
   special case instead. */
static void vbl_autodet_callback(maple_state_t *state, maple_frame_t *frm) {
    maple_response_t    *resp;
    maple_device_t      *dev;
    int         p, u;

    if(irq_inside_int() && !malloc_irq_safe()) {
        /* We can't create or remove a device now. Fail silently as the device
         * will be re-probed in the next loop of the periodic IRQ. */
        maple_frame_unlock(frm);
        return;
    }

    /* So.. did we get a response? */
    resp = (maple_response_t *)frm->recv_buf;
    p = frm->dst_port;
    u = frm->dst_unit;
    dev = maple_enum_dev(p, u);

    if(resp->response == MAPLE_RESPONSE_NONE) {
        /* No device, or not functioning properly; check for removal */
        if(u == 0) {
            if(dev) {
                /* Top-level device -- detach all sub-devices as well */
                for(u = 0; u < MAPLE_UNIT_COUNT; u++) {
                    vbl_chk_disconnect(state, p, u);
                }

                dev->dev_mask = 0;
            }

            state->scan_ready_mask |= 1 << p;
        }
        else {
            /* Not a top-level device -- only detach this device */
            vbl_chk_disconnect(state, p, u);
        }

        maple_frame_unlock(frm);
    }
    else if(resp->response == MAPLE_RESPONSE_DEVINFO) {
        /* Device is present, check for connections */
        if(!dev) {
            if(__is_defined(MAPLE_IRQ_DEBUG)) {
                dbglog(DBG_KDEBUG, "maple: attach on device %c%c\n",
                       'A' + p, '0' + u);
            }

            if(maple_driver_attach(frm) == 0) {
                assert(maple_dev_valid(p, u));
            }
        }
        else {
            maple_devinfo_t     *devinfo;
            /* Device already connected, update function data (caps) */
            devinfo = (maple_devinfo_t *)resp->data;
            dev->info.function_data[0] = devinfo->function_data[0];
            dev->info.function_data[1] = devinfo->function_data[1];
            dev->info.function_data[2] = devinfo->function_data[2];
        }

        /* If this is a top-level port, then also check any
           sub-devices that claim to be attached */
        if(u == 0)
            vbl_chk_subdevs(state, p, resp->src_addr);
        else
            vbl_dev_probed(p, u);

        maple_frame_unlock(frm);

        /* Probe the next sub-device */
        vbl_chk_next_subdev(state, frm, p);
    }
    else {
        /* dbglog(DBG_KDEBUG, "maple: unknown response %d on device %c%c\n",
            resp->response, 'A'+p, '0'+u); */
        state->scan_ready_mask |= 1 << p;
        maple_frame_unlock(frm);
    }
}

static void vbl_autodetect(maple_state_t *state) {
    bool queued;
    maple_device_t  *dev = maple_state.ports[state->detect_port_next].units[0];
    maple_frame_t   *frm = (dev != NULL) ? &dev->frame : &detect_frame;

    /* Queue a detection on the next device */
    queued = vbl_send_devinfo(frm,
                              state->detect_port_next, 0);

    /* Move to the next device */
    if(queued) {
        state->detect_port_next++;

        if(state->detect_port_next >= MAPLE_PORT_COUNT)
            state->detect_port_next = 0;
    }
}

/* Called on every VBL (~60fps) */
void maple_vbl_irq_hnd(uint32 code, void *data) {
    maple_state_t *state = data;
    maple_driver_t *drv;

    (void)code;

    /* dbgio_write_str("inside vbl_irq_hnd\n"); */

    /* Count, for fun and profit */
    state->vbl_cntr++;

    /* Autodetect changed devices */
    vbl_autodetect(state);

    /* Call all registered drivers' periodic callbacks */
    LIST_FOREACH(drv, &state->driver_list, drv_list) {
        if(drv->periodic != NULL)
            drv->periodic(drv);
    }

    /* Send any queued data */
    if(!state->dma_in_progress)
        maple_queue_flush();

    /* dbgio_write_str("finish vbl_irq_hnd\n"); */
}

/*********************************************************************/
/* Maple DMA completion handler */

/* Called after a Maple DMA send / receive pair completes */
void maple_dma_irq_hnd(uint32 code, void *data) {
    maple_state_t *state = data;
    maple_frame_t   *i, *tmp;
    int8        resp;
    uint32 gun;

    (void)code;

    /* dbgio_write_str("start dma_irq_hnd\n"); */

    /* Count, for fun and profit */
    state->dma_cntr++;

    /* ACK the receipt */
    state->dma_in_progress = 0;

    if(__is_defined(MAPLE_DMA_DEBUG)) {
        maple_sentinel_verify("state->dma_buffer", state->dma_buffer,
                              MAPLE_DMA_SIZE);
    }

    /* For each queued frame, call its callback if it's done */
    TAILQ_FOREACH_SAFE(i, &state->frame_queue, frameq, tmp) {
        /* Skip any unsent or stale items */
        if(i->state != MAPLE_FRAME_SENT)
            continue;

        /* Check to see if it got a proper response; we might
           have to resubmit the request */
        resp = ((int8*)i->recv_buf)[0];

        if(resp == MAPLE_RESPONSE_AGAIN) {
            i->state = MAPLE_FRAME_UNSENT;
            continue;
        }

        if(__is_defined(MAPLE_DMA_DEBUG))
            maple_sentinel_verify("i->recv_buf", i->recv_buf, 1024);

        /* Mark it as responded to */
        i->state = MAPLE_FRAME_RESPONDED;

        maple_queue_remove(i);

        /* If it's got a callback, call it; otherwise unlock
           it manually (or it'll never get used again) */
        if(i->callback != NULL)
            i->callback(state, i);
        else
            maple_frame_unlock(i);
    }

    /* If gun mode is enabled, read the latched H/V counter values. */
    if(state->gun_port > -1) {
        gun = PVR_GET(PVR_GUN_POS);
        state->gun_x = gun & 0x3ff;
        state->gun_y = (gun >> 16) & 0x3ff;
        state->gun_port = -1;
    }

    /* dbgio_write_str("finish dma_irq_hnd\n"); */
}
