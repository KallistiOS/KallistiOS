/* KallistiOS ##version##

   mouse.c
   (C)2002 Megan Potter
*/

#include <dc/maple.h>
#include <dc/maple/mouse.h>
#include <string.h>
#include <assert.h>

static void mouse_reply(maple_state_t *st, maple_frame_t *frm) {
    (void)st;

    maple_response_t    *resp;
    uint32          *respbuf;
    mouse_cond_t        *raw;
    mouse_state_t       *cooked;

    /* Unlock the frame now (it's ok, we're in an IRQ) */
    maple_frame_unlock(frm);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frm->recv_buf;

    if(resp->response != MAPLE_RESPONSE_DATATRF)
        return;

    respbuf = (uint32 *)resp->data;

    if(respbuf[0] != MAPLE_FUNC_MOUSE)
        return;

    if(!frm->dev)
        return;

    /* Verify the size of the frame and grab a pointer to it */
    assert(sizeof(mouse_cond_t) == ((resp->data_len - 1) * sizeof(uint32_t)));
    raw = (mouse_cond_t *)(respbuf + 1);

    /* Fill the "nice" struct from the raw data */
    cooked = (mouse_state_t *)(frm->dev->status);
    cooked->buttons = (~raw->buttons) & 14;
    cooked->dx = raw->dx - MOUSE_DELTA_CENTER;
    cooked->dy = raw->dy - MOUSE_DELTA_CENTER;
    cooked->dz = raw->dz - MOUSE_DELTA_CENTER;
    frm->dev->status_valid = 1;
}

static int mouse_poll(maple_device_t *dev) {
    uint32 * send_buf;

    if(maple_frame_lock(&dev->frame) < 0)
        return 0;

    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_MOUSE;
    dev->frame.cmd = MAPLE_COMMAND_GETCOND;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 1;
    dev->frame.callback = mouse_reply;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    return 0;
}

static void mouse_periodic(maple_driver_t *drv) {
    maple_driver_foreach(drv, mouse_poll);
}

/* Device Driver Struct */
static maple_driver_t mouse_drv = {
    .functions = MAPLE_FUNC_MOUSE,
    .name = "Mouse Driver",
    .periodic = mouse_periodic,
    .status_size = sizeof(mouse_state_t),
    .attach = NULL,
    .detach = NULL
};

/* Add the mouse to the driver chain */
void mouse_init(void) {
    maple_driver_reg(&mouse_drv);
}

void mouse_shutdown(void) {
    maple_driver_unreg(&mouse_drv);
}
