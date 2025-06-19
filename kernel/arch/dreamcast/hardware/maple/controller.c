/* KallistiOS ##version##

   controller.c
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2025 Falco Girgis

 */

#include <arch/arch.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <kos/mutex.h>
#include <kos/worker_thread.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/queue.h>

/* Location of controller capabilities within function_data array */
#define CONT_FUNCTION_DATA_INDEX  0

#ifndef CONT_BTN_CALLBACK_THD_STACK_SIZE
#define CONT_BTN_CALLBACK_THD_STACK_SIZE (8 * 1024)
#endif

/* Raw controller condition structure */
typedef struct cont_cond {
    uint16_t buttons;  /* buttons bitfield */
    uint8_t rtrig;     /* right trigger */
    uint8_t ltrig;     /* left trigger */
    uint8_t joyx;      /* joystick X */
    uint8_t joyy;      /* joystick Y */
    uint8_t joy2x;     /* second joystick X */
    uint8_t joy2y;     /* second joystick Y */
} cont_cond_t;

typedef struct cont_callback_params {
    cont_btn_callback_t cb;
    uint8_t addr;
    uint32_t btns;
    kthread_worker_t *worker;

    uint8_t cur_addr;
    uint32_t cur_btns;

    TAILQ_ENTRY(cont_callback_params)  listent;
} cont_callback_params_t;

static TAILQ_HEAD(cont_btn_callback_list, cont_callback_params) btn_cbs;

static mutex_t btn_cbs_mtx = MUTEX_INITIALIZER;

/* Check whether the controller has EXACTLY the given capabilities. */
int cont_is_type(const maple_device_t *cont, uint32_t type) {
    return cont ? cont->info.function_data[CONT_FUNCTION_DATA_INDEX] == type :
                  -1;
}

/* Check whether the controller has at LEAST the given capabilities. */
int cont_has_capabilities(const maple_device_t *cont, uint32_t capabilities) {
    return cont ? ((cont->info.function_data[CONT_FUNCTION_DATA_INDEX] 
                   & capabilities) == capabilities) : -1;
}

/* This is an internal function for deleting a callback. It happens
    currently in two different ways. Either a NULL callback was
    requested for an addr/btns combination, or we are shutting down
    entirely by passing NULL and cleaning out all entries.

    XXX: This could be useful as part of a more robust Public API
    that would check the rest of the controller joy/trig/dpad and
    just have callback-based input handling.
*/
static void cont_btn_callback_del(cont_callback_params_t *params) {
    cont_callback_params_t *c, *n;

    mutex_lock(&btn_cbs_mtx);

    TAILQ_FOREACH_SAFE(c, &btn_cbs, listent, n) {
        if((params == NULL) || ((params->addr == c->addr) &&
            (params->btns == c->btns))) {

                if((params == NULL) || (params->cb == NULL) ||
                    (params->cb == c->cb)) {
                    TAILQ_REMOVE(&btn_cbs, c, listent);
                    thd_worker_destroy(c->worker);
                    free(c);
                }
            }
    }
    mutex_unlock(&btn_cbs_mtx);
}

static void cont_btn_cb_thread(void *d) {
    cont_callback_params_t *params = d;
    params->cb(params->addr, params->btns);
}

/* Set a controller callback for a button combo; set addr=0 for any controller */
int cont_btn_callback(uint8_t addr, uint32_t btns, cont_btn_callback_t cb) {
    cont_callback_params_t *params;

    params = (cont_callback_params_t *)malloc(sizeof(cont_callback_params_t));

    if(!params) return -1;

    params->addr = addr;
    params->btns = btns;
    params->cb = cb;

    /* This flags us to uninstall the handler for that addr/btn */
    if(cb == NULL) {
        cont_btn_callback_del(params);
        free(params);
        return 0;
    }

    const kthread_attr_t thd_attr = {
        .stack_size = CONT_BTN_CALLBACK_THD_STACK_SIZE,
        .prio = PRIO_DEFAULT,
        .label = "cont_btn_callback"
    };

    params->worker =
        thd_worker_create_ex(&thd_attr, &cont_btn_cb_thread, params);

    if(!params->worker) {
        free(params);
        return -1;
    }

    mutex_lock(&btn_cbs_mtx);

    if(addr)
        TAILQ_INSERT_HEAD(&btn_cbs, params, listent);
    else
        TAILQ_INSERT_TAIL(&btn_cbs, params, listent);

    mutex_unlock(&btn_cbs_mtx);

    return 0;
}

/* Response callback for the GETCOND Maple command. */
static void cont_reply(maple_state_t *st, maple_frame_t *frm) {
    (void)st;

    maple_response_t *resp;
    uint32_t         *respbuf;
    cont_cond_t      *raw;
    cont_state_t     *cooked;
    cont_callback_params_t *c;

    /* Unlock the frame now (it's ok, we're in an IRQ) */
    maple_frame_unlock(frm);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frm->recv_buf;

    if(resp->response != MAPLE_RESPONSE_DATATRF)
        return;

    respbuf = (uint32_t *)resp->data;

    if(respbuf[0] != MAPLE_FUNC_CONTROLLER)
        return;

    if(!frm->dev)
        return;

    /* Verify the size of the frame and grab a pointer to it */
    assert(sizeof(cont_cond_t) == ((resp->data_len - 1) * sizeof(uint32_t)));
    raw = (cont_cond_t *)(respbuf + 1);

    /* Fill the "nice" struct from the raw data */
    cooked = (cont_state_t *)(frm->dev->status);
    cooked->buttons = (~raw->buttons) & 0xffff;
    cooked->ltrig = raw->ltrig;
    cooked->rtrig = raw->rtrig;
    cooked->joyx = ((int)raw->joyx) - 128;
    cooked->joyy = ((int)raw->joyy) - 128;
    cooked->joy2x = ((int)raw->joy2x) - 128;
    cooked->joy2y = ((int)raw->joy2y) - 128;
    frm->dev->status_valid = 1;

    /* If someone is in the middle of modifying the list, don't process callbacks */
    if(mutex_trylock(&btn_cbs_mtx))
        return;

    /* Check for magic button sequences */
    TAILQ_FOREACH(c, &btn_cbs, listent) {
        if(!c->addr ||
                (c->addr &&
                 c->addr == maple_addr(frm->dev->port, frm->dev->unit))) {
            if((cooked->buttons & c->btns) == c->btns) {
                c->cur_btns = cooked->buttons;
                c->cur_addr = maple_addr(frm->dev->port, frm->dev->unit);
                thd_worker_wakeup(c->worker);
            }
        }
    }

    mutex_unlock(&btn_cbs_mtx);
}

static int cont_poll(maple_device_t *dev) {
    uint32_t *send_buf;

    if(maple_frame_lock(&dev->frame) < 0)
        return 0;

    maple_frame_init(&dev->frame);
    send_buf = (uint32_t *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_CONTROLLER;
    dev->frame.cmd = MAPLE_COMMAND_GETCOND;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 1;
    dev->frame.callback = cont_reply;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    return 0;
}

static void cont_periodic(maple_driver_t *drv) {
    maple_driver_foreach(drv, cont_poll);
}

/* Device Driver Struct */
static maple_driver_t controller_drv = {
    .functions = MAPLE_FUNC_CONTROLLER,
    .name = "Controller Driver",
    .periodic = cont_periodic,
    .status_size = sizeof(cont_state_t),
    .attach = NULL,
    .detach = NULL
};

/* Add the controller to the driver chain */
void cont_init(void) {
    TAILQ_INIT(&btn_cbs);
    maple_driver_reg(&controller_drv);
}

void cont_shutdown(void) {
    /* Empty the callback list */
    cont_btn_callback_del(NULL);
    maple_driver_unreg(&controller_drv);
}
