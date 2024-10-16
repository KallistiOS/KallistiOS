/* KallistiOS ##version##

   dreameye.c
   Copyright (C) 2005, 2009 Lawrence Sebald
*/

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <kos/dbglog.h>
#include <kos/genwait.h>
#include <dc/maple.h>
#include <dc/maple/dreameye.h>

static int dreameye_send_get_image(maple_device_t *dev,
                                   dreameye_state_t *state, uint8 req,
                                   uint8 cnt);

static dreameye_state_t *first_state = NULL;

static void dreameye_get_image_count_cb(maple_state_t *st, maple_frame_t *frame) {
    (void)st;

    dreameye_state_t *de;
    maple_response_t *resp;
    uint32 *respbuf32;
    uint8 *respbuf8;

    /* Unlock the frame */
    maple_frame_unlock(frame);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frame->recv_buf;

    if(resp->response != MAPLE_RESPONSE_DATATRF)
        return;

    respbuf32 = (uint32 *)resp->data;
    respbuf8 = (uint8 *)resp->data;

    if(respbuf32[0] != MAPLE_FUNC_CAMERA)
        return;

    /* Update the status that was requested. */
    if(frame->dev) {
        assert((resp->data_len) == 3);
        assert(respbuf8[4] == 0xD0);
        assert(respbuf8[5] == 0x00);
        assert(respbuf8[8] == DREAMEYE_GETCOND_NUM_IMAGES);
        assert(respbuf8[9] == 0x04);

        /* Update the data in the status. */
        de = (dreameye_state_t *)frame->dev->status;
        de->image_count = (respbuf8[10] << 8) | respbuf8[11];
        de->image_count_valid = 1;
        frame->dev->status_valid = 1;
    }

    /* Wake up! */
    genwait_wake_all(frame);
}

static void dreameye_get_transfer_count_cb(maple_state_t *st, maple_frame_t *frame) {
    (void)st;

    dreameye_state_t *de;
    maple_response_t *resp;
    uint32 *respbuf32;
    uint8 *respbuf8;

    /* Unlock the frame */
    maple_frame_unlock(frame);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frame->recv_buf;

    if(resp->response != MAPLE_RESPONSE_DATATRF)
        return;

    respbuf32 = (uint32 *)resp->data;
    respbuf8 = (uint8 *)resp->data;

    if(respbuf32[0] != MAPLE_FUNC_CAMERA)
        return;

    /* Update the status that was requested. */
    if(frame->dev) {
        assert((resp->data_len) == 3);
        assert(respbuf8[4] == 0xD0);
        assert(respbuf8[5] == 0x00);
        assert(respbuf8[8] == DREAMEYE_GETCOND_TRANSFER_COUNT);

        /* Update the data in the status. */
        de = (dreameye_state_t *)frame->dev->status;
        de->transfer_count = (respbuf8[10] << 8) | respbuf8[11];
    }

    /* Wake up! */
    genwait_wake_all(frame);
}

int dreameye_get_image_count(maple_device_t *dev, int block) {
    dreameye_state_t *de;
    uint32 *send_buf;

    assert(dev != NULL);

    de = (dreameye_state_t *)dev->status;
    de->image_count_valid = 0;

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_CAMERA;
    send_buf[1] = DREAMEYE_GETCOND_NUM_IMAGES | (0x04 << 8);
    dev->frame.cmd = MAPLE_COMMAND_GETCOND;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = dreameye_get_image_count_cb;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    if(block) {
        /* Wait for the Dreameye to accept it */
        if(genwait_wait(&dev->frame, "dreameye_get_image_count", 500,
                        NULL) < 0) {
            if(dev->frame.state != MAPLE_FRAME_VACANT) {
                /* Something went wrong... */
                dev->frame.state = MAPLE_FRAME_VACANT;
                dbglog(DBG_ERROR, "dreameye_get_image_count: timeout to unit "
                       "%c%c\n", dev->port + 'A', dev->unit + '0');
                return MAPLE_ETIMEOUT;
            }
        }
    }

    return MAPLE_EOK;
}

static void dreameye_get_image_cb(maple_state_t *st, maple_frame_t *frame) {
    (void)st;

    maple_device_t *dev;
    maple_response_t *resp;
    uint32 *respbuf32;
    uint8 *respbuf8;
    int len;

    /* Unlock the frame */
    maple_frame_unlock(frame);

    if(frame->dev == NULL)
        return;

    dev = frame->dev;

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frame->recv_buf;

    if(resp->response != MAPLE_RESPONSE_DATATRF) {
        first_state->img_transferring = -1;
        return;
    }

    respbuf32 = (uint32 *)resp->data;
    respbuf8 = (uint8 *)resp->data;

    if(respbuf32[0] != MAPLE_FUNC_CAMERA) {
        first_state->img_transferring = -1;
        return;
    }

    len = (resp->data_len - 3) * 4;

    /* Copy the data. */
    memcpy(first_state->img_buf + (int)(respbuf8[5]) * 512 , respbuf8 + 12,
           len);
    first_state->img_size += len;

    /* Check if we're done. */
    if(respbuf8[4] & 0x40) {
        first_state->img_transferring = 0;
        return;
    }

    if(respbuf8[5] + 5 < first_state->transfer_count)
        dreameye_send_get_image(dev, first_state, DREAMEYE_IMAGEREQ_CONTINUE,
                                respbuf8[5] + 5);
}

static int dreameye_send_get_image(maple_device_t *dev,
                                   dreameye_state_t *state, uint8 req,
                                   uint8 cnt) {
    uint32 *send_buf;

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_CAMERA;
    send_buf[1] = DREAMEYE_SUBCOMMAND_IMAGEREQ | (state->img_number << 8) |
                  (req << 16) | (cnt << 24);
    dev->frame.cmd = MAPLE_COMMAND_CAMCONTROL;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = dreameye_get_image_cb;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    return MAPLE_EOK;
}

static int dreameye_get_transfer_count(maple_device_t *dev, uint8 img) {
    uint32 *send_buf;

    assert(dev != NULL);

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_CAMERA;
    send_buf[1] = DREAMEYE_GETCOND_TRANSFER_COUNT | (img << 8);
    dev->frame.cmd = MAPLE_COMMAND_GETCOND;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = dreameye_get_transfer_count_cb;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    /* Wait for the Dreameye to accept it */
    if(genwait_wait(&dev->frame, "dreameye_get_transfer_count", 500,
                    NULL) < 0) {
        if(dev->frame.state != MAPLE_FRAME_VACANT) {
            /* Something went wrong... */
            dev->frame.state = MAPLE_FRAME_VACANT;
            dbglog(DBG_ERROR, "dreameye_get_transfer_count: timeout to unit "
                   "%c%c\n", dev->port + 'A', dev->unit + '0');
            return MAPLE_ETIMEOUT;
        }
    }

    return MAPLE_EOK;
}

int dreameye_get_image(maple_device_t *dev, uint8 image, uint8 **data,
                       int *img_sz) {
    dreameye_state_t *de;
    maple_device_t *dev2, *dev3, *dev4, *dev5;
    int err;

    assert(dev != NULL);
    assert(dev->unit == 1);

    dev2 = maple_enum_dev(dev->port, 2);
    dev3 = maple_enum_dev(dev->port, 3);
    dev4 = maple_enum_dev(dev->port, 4);
    dev5 = maple_enum_dev(dev->port, 5);

    de = (dreameye_state_t *)dev->status;

    first_state = de;
    de->img_transferring = 1;
    de->img_buf = NULL;
    de->img_size = 0;
    de->img_number = image;
    de->transfer_count = 0;

    /* Figure out how many transfers we'll need. */
    err = dreameye_get_transfer_count(dev, image);

    if(err)
        goto fail;

    /* Allocate space for the largest possible image that could fit in that
       number of transfers. */
    de->img_buf = (uint8 *)malloc(512 * de->transfer_count);

    if(!de->img_buf)
        goto fail;

    /* Send out the image requests to all sub devices. */
    dreameye_send_get_image(dev, de, DREAMEYE_IMAGEREQ_START, 0);
    dreameye_send_get_image(dev2, de, DREAMEYE_IMAGEREQ_CONTINUE, 1);
    dreameye_send_get_image(dev3, de, DREAMEYE_IMAGEREQ_CONTINUE, 2);
    dreameye_send_get_image(dev4, de, DREAMEYE_IMAGEREQ_CONTINUE, 3);
    dreameye_send_get_image(dev5, de, DREAMEYE_IMAGEREQ_CONTINUE, 4);

    while(de->img_transferring == 1) {
        thd_pass();
    }

    if(de->img_transferring == 0) {
        *data = de->img_buf;
        *img_sz = de->img_size;

        dbglog(DBG_DEBUG, "dreameye_get_image: Image of size %d received in "
               "%d transfers\n", de->img_size, de->transfer_count);

        first_state = NULL;
        de->img_buf = NULL;
        de->img_size = 0;
        de->transfer_count = 0;
        return MAPLE_EOK;
    }

    /* If we get here, something went wrong. */
    if(de->img_buf != NULL) {
        free(de->img_buf);
    }

fail:
    first_state = NULL;
    de->img_transferring = 0;
    de->img_buf = NULL;
    de->img_size = 0;
    de->transfer_count = 0;

    return MAPLE_EFAIL;
}

static void dreameye_erase_cb(maple_state_t *st, maple_frame_t *frame) {
    (void)st;

    maple_response_t *resp;
    uint8 *respbuf;

    /* Unlock the frame */
    maple_frame_unlock(frame);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frame->recv_buf;
    respbuf = (uint8 *)resp->data;

    if(resp->response == MAPLE_COMMAND_CAMCONTROL &&
            respbuf[4] == DREAMEYE_SUBCOMMAND_ERROR) {
        dbglog(DBG_ERROR, "dreameye_erase_image: Dreameye returned error code "
               "0x%02X%02X%02X\n", respbuf[5], respbuf[6], respbuf[7]);
    }
    else if(resp->response != MAPLE_RESPONSE_OK)
        return;

    /* Wake up! */
    genwait_wake_all(frame);
}

int dreameye_erase_image(maple_device_t *dev, uint8 image, int block) {
    uint32 *send_buf;

    assert(dev != NULL);

    if(image < 0x02 || (image > 0x21 && image != 0xFF))
        return MAPLE_EINVALID;

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_CAMERA;
    send_buf[1] = DREAMEYE_SUBCOMMAND_ERASE | (0x80 << 8) | (image << 16);
    dev->frame.cmd = MAPLE_COMMAND_CAMCONTROL;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = dreameye_erase_cb;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    if(block) {
        /* Wait for the Dreameye to accept it */
        if(genwait_wait(&dev->frame, "dreameye_erase_image", 500, NULL) < 0) {
            if(dev->frame.state != MAPLE_FRAME_VACANT) {
                /* Something went wrong.... */
                dev->frame.state = MAPLE_FRAME_VACANT;
                dbglog(DBG_ERROR, "dreameye_erase_image: timeout to unit "
                       "%c%c\n", dev->port + 'A', dev->unit + '0');
                return MAPLE_ETIMEOUT;
            }
        }
    }

    return MAPLE_EOK;
}

static int dreameye_poll(maple_device_t *dev) {
    /* For right now, we don't have anything particularly pressing to do here,
       so punt. */
    dev->status_valid = 1;
    return 0;
}

static void dreameye_periodic(maple_driver_t *drv) {
    maple_driver_foreach(drv, dreameye_poll);
}

static int dreameye_attach(maple_driver_t *drv, maple_device_t *dev) {
    dreameye_state_t *de;

    (void)drv;

    de = (dreameye_state_t *)dev->status;
    de->image_count = 0;
    de->image_count_valid = 0;
    de->transfer_count = 0;
    de->img_transferring = 0;
    de->img_buf = NULL;
    de->img_size = 0;
    de->img_number = 0;

    dev->status_valid = 1;
    return 0;
}

/* Device Driver Struct */
static maple_driver_t dreameye_drv = {
    .functions = MAPLE_FUNC_CAMERA,
    .name = "Dreameye (Camera)",
    .periodic = dreameye_periodic,
    .status_size = sizeof(dreameye_state_t),
    .attach = dreameye_attach,
    .detach = NULL
};

/* Add the Dreameye to the driver chain */
void dreameye_init(void) {
    maple_driver_reg(&dreameye_drv);
}

void dreameye_shutdown(void) {
    maple_driver_unreg(&dreameye_drv);
}
