/* KallistiOS ##version##

   purupuru.c
   Copyright (C) 2003 Megan Potter
   Copyright (C) 2005 Lawrence Sebald
*/

#include <assert.h>
#include <kos/dbglog.h>
#include <kos/genwait.h>
#include <dc/maple.h>
#include <dc/maple/purupuru.h>

/* Be warned, not all purus are created equal, in fact, most of
   them act different for just about everything you feed to them. */

static void purupuru_rumble_cb(maple_state_t *st, maple_frame_t *frame) {
    (void)st;

    /* Unlock the frame */
    maple_frame_unlock(frame);

    /* Wake up! */
    genwait_wake_all(frame);
}

int purupuru_rumble_raw(maple_device_t *dev, uint32 effect) {
    uint32 *send_buf;

    assert(dev != NULL);

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_PURUPURU;
    send_buf[1] = effect;
    dev->frame.cmd = MAPLE_COMMAND_SETCOND;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = purupuru_rumble_cb;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    /* Wait for the purupuru to accept it */
    if(genwait_wait(&dev->frame, "purupuru_rumble", 500, NULL) < 0) {
        if(dev->frame.state != MAPLE_FRAME_VACANT) {
            /* Something went wrong.... */
            dev->frame.state = MAPLE_FRAME_VACANT;
            dbglog(DBG_ERROR, "purupuru_rumble: timeout to unit %c%c\n",
                   dev->port + 'A', dev->unit + '0');
            return MAPLE_ETIMEOUT;
        }
    }

    return MAPLE_EOK;
}

int purupuru_rumble(maple_device_t *dev, purupuru_effect_t *effect) {
    uint32 comp_effect;

    assert(dev != NULL);

    /* "Compile" the effect */
    comp_effect = (effect->duration << 24) | (effect->effect2 << 16) |
                  (effect->effect1 << 8) | (effect->special);

    return purupuru_rumble_raw(dev, comp_effect);
}


/* Device Driver Struct */
static maple_driver_t purupuru_drv = {
    .functions = MAPLE_FUNC_PURUPURU,
    .name = "PuruPuru (Vibration) Pack",
    .periodic = NULL,
    .attach = NULL,
    .detach = NULL
};

/* Add the purupuru to the driver chain */
void purupuru_init(void) {
    maple_driver_reg(&purupuru_drv);
}

void purupuru_shutdown(void) {
    maple_driver_unreg(&purupuru_drv);
}
