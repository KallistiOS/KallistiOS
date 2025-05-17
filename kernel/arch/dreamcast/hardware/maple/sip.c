/* KallistiOS ##version##

   sip.c
   Copyright (C) 2005, 2008, 2013 Lawrence Sebald
*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arch/irq.h>
#include <kos/dbglog.h>
#include <kos/genwait.h>
#include <dc/maple.h>
#include <dc/maple/sip.h>

#define SIP_START_SAMPLING 0x80

static void sip_start_sampling_cb(maple_state_t *st, maple_frame_t *frame) {
    (void)st;

    sip_state_t *sip;
    maple_response_t *resp;

    /* Unlock the frame */
    maple_frame_unlock(frame);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frame->recv_buf;

    if(resp->response != MAPLE_RESPONSE_OK)
        return;

    /* Set the is_sampling flag. */
    sip = (sip_state_t *)frame->dev->status;
    sip->is_sampling = 1;

    /* Wake up! */
    genwait_wake_all(frame);
}

static void sip_stop_sampling_cb(maple_state_t *st, maple_frame_t *frame) {
    (void)st;

    sip_state_t *sip;
    maple_response_t *resp;

    /* Unlock the frame */
    maple_frame_unlock(frame);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frame->recv_buf;

    if(resp->response != MAPLE_RESPONSE_OK)
        return;

    /* Clear the is_sampling flag. */
    sip = (sip_state_t *)frame->dev->status;
    sip->is_sampling = 0;
    sip->callback = NULL;

    /* Wake up! */
    genwait_wake_all(frame);
}

int sip_set_gain(maple_device_t *dev, unsigned int g) {
    sip_state_t *sip;

    assert(dev != NULL);

    /* Check the gain value for validity */
    if(g > SIP_MAX_GAIN)
        return MAPLE_EINVALID;

    sip = (sip_state_t *)dev->status;
    sip->amp_gain = g;

    return MAPLE_EOK;
}

int sip_set_sample_type(maple_device_t *dev, unsigned int type) {
    sip_state_t *sip;

    assert(dev != NULL);

    /* Check the sample type value for validity. */
    if(type > SIP_SAMPLE_8BIT_ULAW)
        return MAPLE_EINVALID;

    sip = (sip_state_t *)dev->status;

    /* Make sure we aren't sampling already. */
    if(sip->is_sampling)
        return MAPLE_EFAIL;

    sip->sample_type = type;

    return MAPLE_EOK;
}

int sip_set_frequency(maple_device_t *dev, unsigned int freq) {
    sip_state_t *sip;

    assert(dev != NULL);

    /* Check the frequency value for validity. */
    if(freq > SIP_SAMPLE_8KHZ)
        return MAPLE_EINVALID;

    sip = (sip_state_t *)dev->status;

    /* Make sure we aren't sampling already. */
    if(sip->is_sampling)
        return MAPLE_EFAIL;

    sip->frequency = freq;

    return MAPLE_EOK;
}

int sip_start_sampling(maple_device_t *dev, sip_sample_cb cb, int block) {
    sip_state_t *sip;
    uint32 *send_buf;

    assert(dev != NULL);

    sip = (sip_state_t *)dev->status;

    /* Make sure we aren't yet sampling and that the callback is sane. */
    if(sip->is_sampling || !cb)
        return MAPLE_EFAIL;

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    sip->callback = cb;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_MICROPHONE;
    send_buf[1] = SIP_SUBCOMMAND_BASIC_CTRL |
                  (((sip->sample_type) | (sip->frequency << 2) |
                    SIP_START_SAMPLING) << 8);
    dev->frame.cmd = MAPLE_COMMAND_MICCONTROL;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = sip_start_sampling_cb;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    if(block) {
        /* Wait for the SIP to accept it */
        if(genwait_wait(&dev->frame, "sip_start_sampling", 500, NULL) < 0) {
            if(dev->frame.state != MAPLE_FRAME_VACANT) {
                /* Something went wrong.... */
                dev->frame.state = MAPLE_FRAME_VACANT;
                dbglog(DBG_ERROR, "sip_start_sampling: timeout to unit %c%c\n",
                       dev->port + 'A', dev->unit + '0');
                return MAPLE_ETIMEOUT;
            }
        }
    }

    return MAPLE_EOK;
}

int sip_stop_sampling(maple_device_t *dev, int block) {
    sip_state_t *sip;
    uint32 *send_buf;

    assert(dev != NULL);

    sip = (sip_state_t *)dev->status;

    /* Make sure we actually are sampling */
    if(!sip->is_sampling)
        return MAPLE_EFAIL;

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_MICROPHONE;
    send_buf[1] = SIP_SUBCOMMAND_BASIC_CTRL;
    dev->frame.cmd = MAPLE_COMMAND_MICCONTROL;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = sip_stop_sampling_cb;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    if(block) {
        /* Wait for the SIP to accept it */
        if(genwait_wait(&dev->frame, "sip_stop_sampling", 500, NULL) < 0) {
            if(dev->frame.state != MAPLE_FRAME_VACANT) {
                /* Something went wrong.... */
                dev->frame.state = MAPLE_FRAME_VACANT;
                dbglog(DBG_ERROR, "sip_stop_sampling: timeout to unit %c%c\n",
                       dev->port + 'A', dev->unit + '0');
                return MAPLE_ETIMEOUT;
            }
        }
    }

    return MAPLE_EOK;
}

static void sip_reply(maple_state_t *st, maple_frame_t *frm) {
    (void)st;

    maple_response_t *resp;
    uint32 *respbuf;
    sip_state_t *sip;

    /* Unlock the frame now (it's ok, we're in an IRQ) */
    maple_frame_unlock(frm);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frm->recv_buf;

    if(resp->response != MAPLE_RESPONSE_DATATRF)
        return;

    respbuf = (uint32 *)resp->data;

    if(respbuf[0] != MAPLE_FUNC_MICROPHONE)
        return;

    if(!frm->dev)
        return;

    sip = (sip_state_t *)frm->dev->status;
    frm->dev->status_valid = 1;

    if(sip->is_sampling && sip->callback) {
        /* Call the user's callback. */
        sip->callback(frm->dev, resp->data + 8, (resp->data_len << 2) - 8);
    }
}

static int sip_poll(maple_device_t *dev) {
    sip_state_t *sip;
    uint32 *send_buf;

    sip = (sip_state_t *)dev->status;

    /* Test to make sure that the particular mic is enabled */
    if(!sip->is_sampling || !sip->callback) {
        dev->status_valid = 1;
        return 0;
    }

    /* Lock the frame, or die trying */
    if(maple_frame_lock(&dev->frame) < 0)
        return 0;

    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_MICROPHONE;
    send_buf[1] = SIP_SUBCOMMAND_GET_SAMPLES |
                  (sip->amp_gain << 8);
    dev->frame.cmd = MAPLE_COMMAND_MICCONTROL;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = sip_reply;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    return 0;
}

static void sip_periodic(maple_driver_t *drv) {
    maple_driver_foreach(drv, sip_poll);
}

static int sip_attach(maple_driver_t *drv, maple_device_t *dev) {
    sip_state_t *sip;

    (void)drv;

    sip = (sip_state_t *)dev->status;
    sip->is_sampling = 0;
    sip->amp_gain = SIP_DEFAULT_GAIN;
    sip->callback = NULL;
    dev->status_valid = 1;

    return 0;
}

/* Device Driver Struct */
static maple_driver_t sip_drv = {
    .functions = MAPLE_FUNC_MICROPHONE,
    .name = "Sound Input Peripheral",
    .periodic = sip_periodic,
    .status_size = sizeof(sip_state_t),
    .attach = sip_attach,
    .detach = NULL
};

/* Add the SIP to the driver chain */
void sip_init(void) {
    maple_driver_reg(&sip_drv);
}

void sip_shutdown(void) {
    maple_driver_unreg(&sip_drv);
}
