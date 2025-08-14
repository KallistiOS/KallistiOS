/* KallistiOS ##version##

   purupuru.c
   Copyright (C) 2003 Megan Potter
   Copyright (C) 2005 Lawrence Sebald
   Copyright (C) 2025 Donald Haase
*/

#include <assert.h>
#include <kos/dbglog.h>
#include <dc/maple.h>
#include <dc/maple/purupuru.h>

/* Be warned, not all purus are created equal, in fact, most of
   them act different for just about everything you feed to them. */

int purupuru_rumble_raw(maple_device_t *dev, uint32_t effect) {
    uint32_t *send_buf;

    assert(dev != NULL);

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32_t *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_PURUPURU;
    send_buf[1] = effect;
    dev->frame.cmd = MAPLE_COMMAND_SETCOND;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = NULL;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    return MAPLE_EOK;
}

int purupuru_rumble(maple_device_t *dev, purupuru_effect_t *effect) {

    /* Error checking to prevent hardware-level errors */
    if(!effect->motor) {
        dbglog(DBG_WARNING, "puru: invalid rumble effect sent. motor must be nonzero.\n");
        return MAPLE_EINVALID;
    }

    if(effect->conv && effect->div) {
        dbglog(DBG_WARNING, "puru: invalid rumble effect sent. Divergent and Convergent rumble cannot be set together.\n");
        return MAPLE_EINVALID;
    }

    return purupuru_rumble_raw(dev, effect->raw);
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
