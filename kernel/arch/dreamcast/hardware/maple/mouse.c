/* KallistiOS ##version##

   mouse.c
   (C)2002 Megan Potter
*/

#include <dc/maple.h>
#include <dc/maple/mouse.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

/* Mouse center value in the raw condition structure. */
#define MOUSE_DELTA_CENTER      0x200

/* Raw controller condition structure */
typedef struct {
    uint16_t    buttons;
    uint16_t    dummy1;
    int16_t     dx;
    int16_t     dy;
    int16_t     dz;
    uint16_t    dummy2;
    uint32_t    dummy3;
    uint32_t    dummy4;
} mouse_cond_t;

static void mouse_reply(maple_state_t *st, maple_frame_t *frm) {
    (void)st;

    maple_response_t    *resp;
    uint32_t            *respbuf;
    mouse_cond_t        *raw;
    mouse_state_t       *cooked;

    /* Unlock the frame now (it's ok, we're in an IRQ) */
    maple_frame_unlock(frm);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frm->recv_buf;

    if(resp->response != MAPLE_RESPONSE_DATATRF)
        return;

    respbuf = (uint32_t *)resp->data;

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
}

static int mouse_poll(maple_device_t *dev) {
    if(maple_frame_trylock(&dev->frame) < 0)
        return 0;

    maple_frame_init(&dev->frame);
    dev->frame.send_buf[0] = MAPLE_FUNC_MOUSE;
    dev->frame.cmd = MAPLE_COMMAND_GETCOND;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 1;
    dev->frame.callback = mouse_reply;
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
    .status_size = sizeof(mouse_state_t)
};

/* Add the mouse to the driver chain */
void mouse_init(void) {
    maple_driver_reg(&mouse_drv);
}

void mouse_shutdown(void) {
    maple_driver_unreg(&mouse_drv);
}
