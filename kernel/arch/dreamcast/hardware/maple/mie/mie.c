/* KallistiOS ##version##

   mie.c
   Copyright (C) 2026 Ruslan Rostovtsev

   MIE (Maple I/O Emulator) driver for the JVS bridge on port A.

   Two Maple frames share the bus:
     - dev->frame  — periodic input poll and deferred coin/output commands
     - mie_cmd_frame — blocking init/eeprom/fw commands (mie_cmd_resp)

   Input poll is a two-step IO transaction handled in mie_poll/mie_reply:
     1) send bundled JVS read (SWINP + COININP + ANLINP)
     2) fetch IOR buffer and parse via mie_apply_buttons
*/

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdalign.h>
#include <assert.h>
#include <arch/arch.h>

#include <dc/maple.h>
#include <dc/maple/mie.h>
#include <dc/maple/controller.h>

#include "mie_internal.h"

#include <kos/dbglog.h>
#include <kos/genwait.h>
#include <kos/mutex.h>
#include <kos/regfield.h>
#include <kos/thread.h>
#include <kos/timer.h>

#define MIE_MAPLE_PORT 0
#define MIE_MAPLE_UNIT 0

#define MIE_CMD_GET_ID   0x82
#define MIE_RESP_GET_ID  0x83
#define MIE_CMD_IO       0x86
#define MIE_RESP_IO      0x87

#define MIE_MAPLE_IDLE_PROBE MAPLE_COMMAND_DEVINFO

#define MIE_IOR_JVS_EMPTY 0x32
#define MIE_JVS_FETCH_MIN_WORDS 8

#define MIE_COIN_CMD_TX_WORDS 3

#define MIE_IO_TIMEOUT_MAX        60
#define MIE_IO_ERR_BACKOFF        30
#define MIE_POLL_IDLE_TIMEOUT_MS  2000

/* Two-phase JVS poll on Maple port A: send bundled request, then fetch result. */
enum {
    MIE_IO_IDLE = 0,
    MIE_IO_WAIT_ACK,
    MIE_IO_NEED_FETCH,
    MIE_IO_WAIT_FETCH
};

typedef enum {
    MIE_DEF_CMD_PENDING = 0,
    MIE_DEF_CMD_OK = 1,
    MIE_DEF_CMD_ERR = -1
} mie_def_cmd_state_t;

static int mie_attach(maple_driver_t *drv, maple_device_t *dev);
static void mie_detach(maple_driver_t *drv, maple_device_t *dev);
static void mie_periodic(maple_driver_t *drv);

static maple_driver_t mie_drv = {
    .functions = MAPLE_FUNC_MIE,
    .name = "MIE Driver",
    .periodic = mie_periodic,
    .status_size = sizeof(mie_drv_state_t),
    .attach = mie_attach,
    .detach = mie_detach,
};
static maple_frame_t mie_cmd_frame;
static mie_drv_state_t mie_static_state;
/* Synthetic device when nothing else occupies port A unit 0. */
static maple_device_t mie_static_dev = {
    .port = MIE_MAPLE_PORT,
    .unit = MIE_MAPLE_UNIT,
    .frame = { .state = MAPLE_FRAME_VACANT },
    .status = &mie_static_state,
    .info = {
        .functions = MAPLE_FUNC_MIE,
        .product_name = "MIE JVS Bridge",
        .product_license = "SEGA ENTERPRISES,LTD.",
        .area_code = 0xff,
    },
};
alignas(32) static uint8_t mie_io_tx_arr[32];
/* Port A role: unknown until GET_ID, then JVS bridge or plain Maple. */
static mie_port0_mode_t port0_mode = MIE_PORT0_UNKNOWN;
static bool mie_jvs_ready;       /* periodic poll enabled after Z80 init */
static bool mie_z80_init_done;
static bool mie_drv_registered;
static uint32_t mie_jvs_output_cache;
static mutex_t mie_bus_mtx = MUTEX_INITIALIZER;
static volatile bool mie_bus_suspend; /* blocks new polls during sync IO */

typedef struct mie_cmd_req {
    uint8_t tx[16];
    uint8_t words;
    volatile mie_def_cmd_state_t state;
} mie_cmd_req_t;

/* Deferred coin/output commands queued by API, sent on next idle poll cycle. */
static mutex_t mie_def_mtx = MUTEX_INITIALIZER;
static mie_cmd_req_t mie_def_req;
static mie_cmd_req_t *volatile mie_active_cmd;
static volatile bool mie_def_pending;
static int mie_def_wq;

_Static_assert(offsetof(mie_drv_state_t, cont) == offsetof(mie_state_t, cont),
               "mie_drv_state_t layout must match mie_state_t");
_Static_assert(offsetof(mie_drv_state_t, jvs) == offsetof(mie_state_t, jvs),
               "mie_drv_state_t layout must match mie_state_t");

static void mie_reply(maple_state_t *st, maple_frame_t *frm);
static void mie_port0_detach(maple_device_t *dev);
static int mie_bind_device(maple_device_t *dev);
static int mie_poll(maple_device_t *dev);
static void mie_bind_port0(void);
static void mie_port0_set_mode(mie_port0_mode_t mode);
static void mie_init_common(const char *fw_path);
static void mie_frame_reset(maple_frame_t *frm);
static maple_response_t *mie_cmd(int cmd, void *send_buf, int length,
                               int expect_resp);

static void mie_frame_reset(maple_frame_t *frm) {
    if(frm->queued) {
        maple_queue_remove(frm);
    }

    frm->state = MAPLE_FRAME_VACANT;
    frm->queued = 0;
}

static int mie_bus_drained(void *data) {
    (void)data;

    return mie_static_state.io_state == MIE_IO_IDLE &&
           !mie_static_dev.frame.queued;
}

bool mie_bus_acquire(void) {
    /* Exclusive bus access for synchronous IO (eeprom, init, etc.).
       The periodic poll uses dev->frame on the same port and must finish first. */
    if(mutex_lock_timed(&mie_bus_mtx, 2000) < 0) {
        return false;
    }

    /* Tell periodic to stop starting new polls and only drain in-flight IO. */
    mie_bus_suspend = true;

    if(thd_poll(mie_bus_drained, NULL, 2000) > 0) {
        return true;
    }

    mie_bus_suspend = false;
    mutex_unlock(&mie_bus_mtx);
    return false;
}

void mie_bus_release(void) {
    mie_bus_suspend = false;
    mutex_unlock(&mie_bus_mtx);
}

static bool mie_cmd_submit(const uint8_t *tx, uint8_t words, bool block) {
    bool ok;

    if(words == 0 || words * 4 > (int)sizeof(mie_def_req.tx)) {
        return false;
    }

    /* One deferred command at a time; poll picks it up on the next idle frame. */
    mutex_lock_scoped(&mie_def_mtx);

    if(mie_def_pending || mie_active_cmd) {
        return false;
    }

    memcpy(mie_def_req.tx, tx, (size_t)words * 4);
    mie_def_req.words = words;
    mie_def_req.state = MIE_DEF_CMD_PENDING;
    mie_def_pending = true;

    /* Async path: poll will send the command; caller does not wait. */
    if(!block) {
        return true;
    }

    /* Sleep until mie_cmd_finish() completes the poll-cycle transaction. */
    if(mie_def_req.state == MIE_DEF_CMD_PENDING) {
        genwait_wait(&mie_def_wq, "mie_cmd", 2000);
    }

    ok = mie_def_req.state == MIE_DEF_CMD_OK;
    /* Drop the request so periodic resumes normal input polling. */
    mie_def_pending = false;
    /* On timeout poll may still hold a stale pointer to this request. */
    if(mie_active_cmd == &mie_def_req) {
        mie_active_cmd = NULL;
    }
    return ok;
}

static int mie_queue_io(maple_device_t *dev, void (*build)(uint8_t *),
                        int length, uint8_t next_state) {
    mie_drv_state_t *ms;

    assert(dev);
    assert(dev->status);
    assert(build);

    ms = dev->status;

    if(dev->frame.queued && dev->frame.state == MAPLE_FRAME_VACANT) {
        mie_frame_reset(&dev->frame);
    }

    if(maple_frame_trylock(&dev->frame) < 0) {
        return -1;
    }

    maple_frame_init(&dev->frame);
    build((uint8_t *)dev->frame.send_buf);
    dev->frame.cmd = MIE_CMD_IO;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = length;
    dev->frame.callback = mie_reply;
    dev->frame.dev = dev;
    ms->io_state = next_state;
    return maple_queue_frame(&dev->frame);
}

/* Periodic bundled JVS input read; skipped while io_backoff counts down. */
static int mie_queue_buttons_request(maple_device_t *dev) {
    mie_drv_state_t *ms = dev->status;


    if(ms->io_state != MIE_IO_IDLE || ms->io_backoff > 0) {
        return -1;
    }

    return mie_queue_io(dev, mie_build_buttons_tx, mie_jvs_poll_tx_words(),
                        MIE_IO_WAIT_ACK);
}

static int mie_queue_jvs_fetch(maple_device_t *dev) {
    return mie_queue_io(dev, mie_build_jvs_fetch, 1, MIE_IO_WAIT_FETCH);
}

static void mie_build_active_cmd(uint8_t *buf) {
    mie_cmd_req_t *req = mie_active_cmd;

    memcpy(buf, req->tx, (size_t)req->words * 4);
}

static int mie_queue_cmd(maple_device_t *dev) {
    return mie_queue_io(dev, mie_build_active_cmd, mie_active_cmd->words,
                        MIE_IO_WAIT_ACK);
}

/* Complete a blocking deferred command and wake mie_cmd_submit waiter. */
static void mie_cmd_finish(mie_def_cmd_state_t result) {
    if(!mie_active_cmd) {
        return;
    }

    mie_active_cmd->state = result;
    mie_active_cmd = NULL;
    genwait_wake_all(&mie_def_wq);
}


/* Maple callback for periodic IO. Advances io_state and applies input on fetch. */
static void mie_reply(maple_state_t *st, maple_frame_t *frm) {
    maple_response_t *resp;
    maple_device_t *dev;
    mie_drv_state_t *ms;
    uint8_t *payload;
    uint8_t *tx;
    int payload_len;

    (void)st;

    assert(frm);
    dev = frm->dev;
    assert(dev);
    assert(dev->status);

    ms = dev->status;
    resp = (maple_response_t *)frm->recv_buf;

    /* Bus busy — retry fetch or drop back to idle. */
    if(resp->response == MAPLE_RESPONSE_AGAIN) {
        maple_frame_unlock(frm);
        if(ms->io_state == MIE_IO_WAIT_FETCH) {
            ms->io_state = MIE_IO_NEED_FETCH;
        }
        else {
            ms->io_state = MIE_IO_IDLE;
        }
        return;
    }

    maple_frame_unlock(frm);

    /* Unexpected response code — may still be a partial IOR echo of send_buf[0]. */
    if((uint8_t)resp->response != MIE_RESP_IO) {
        tx = (uint8_t *)frm->send_buf;

        if(ms->io_state == MIE_IO_WAIT_FETCH &&
                tx && (uint8_t)resp->response == tx[0]) {
            ms->io_state = MIE_IO_NEED_FETCH;
            return;
        }

        /* One-shot deferred cmd (coin/output) acked without fetch phase. */
        if(ms->io_state == MIE_IO_WAIT_ACK &&
                tx && (uint8_t)resp->response == tx[0]) {
            mie_cmd_finish(MIE_DEF_CMD_OK);
            ms->io_state = MIE_IO_IDLE;
            return;
        }

        if(resp->response != MAPLE_RESPONSE_NONE) {
            ms->io_backoff = MIE_IO_ERR_BACKOFF;
        }
        mie_cmd_finish(MIE_DEF_CMD_ERR);
        ms->io_state = MIE_IO_IDLE;
        return;
    }

    payload = (uint8_t *)resp->data;
    payload_len = (int)resp->data_len * 4;

    /* Fetch phase: Z80 may return empty until JVS data is ready. */
    if(ms->io_state == MIE_IO_WAIT_FETCH) {
        if(payload_len >= 1 && payload[0] == MIE_IOR_JVS_EMPTY) {
            ms->io_state = MIE_IO_NEED_FETCH;
            return;
        }

        if(resp->data_len < MIE_JVS_FETCH_MIN_WORDS) {
            ms->io_state = MIE_IO_NEED_FETCH;
            return;
        }

        if(mie_active_cmd) {
            mie_cmd_finish(MIE_DEF_CMD_OK);
        }
        else {
            mie_apply_buttons(resp, dev);
        }

        ms->io_state = MIE_IO_IDLE;
        return;
    }

    /* Request accepted — schedule fetch on next periodic tick. */
    if(ms->io_state == MIE_IO_WAIT_ACK) {
        ms->io_state = MIE_IO_NEED_FETCH;
        return;
    }

    ms->io_state = MIE_IO_IDLE;
}

static int mie_poll(maple_device_t *dev) {
    mie_drv_state_t *ms;

    assert(dev);
    assert(dev->status);

    ms = dev->status;

    /* Continue a poll cycle waiting for JVS fetch data. */
    if(ms->io_state == MIE_IO_NEED_FETCH) {
        return mie_queue_jvs_fetch(dev);
    }

    /* Mid-transaction or error backoff — wait for mie_reply / mie_io_tick. */
    if(ms->io_state != MIE_IO_IDLE || ms->io_backoff > 0) {
        return -1;
    }

    /* Retry a deferred command after AGAIN or partial fetch. */
    if(mie_active_cmd) {
        return mie_queue_cmd(dev);
    }

    /* Pick up a new deferred command (coin add, output, etc.). */
    if(mie_def_pending) {
        mie_active_cmd = &mie_def_req;
        mie_def_pending = false;
        return mie_queue_cmd(dev);
    }

    /* Normal input poll: SWINP + coin + analog bundled request. */
    return mie_queue_buttons_request(dev);
}

static int mie_attach(maple_driver_t *drv, maple_device_t *dev) {
    mie_drv_state_t *ms;

    (void)drv;

    ms = dev->status;
    memset(ms, 0, sizeof(*ms));
    ms->io_state = MIE_IO_IDLE;
    return 0;
}

static void mie_detach(maple_driver_t *drv, maple_device_t *dev) {
    (void)drv;
    (void)dev;
}

static int mie_bind_device(maple_device_t *dev) {
    mie_drv_state_t *ms;

    if(dev->drv == &mie_drv) {
        return 1;
    }

    if(!dev->status) {
        if(dev != &mie_static_dev) {
            return -1;
        }
        dev->status = &mie_static_state;
    }

    if(mie_attach(&mie_drv, dev) < 0) {
        return -1;
    }

    dev->drv = &mie_drv;
    dev->valid = true;

    if(dev->status) {
        ms = dev->status;

        memset(&ms->jvs.coin, 0, sizeof(ms->jvs.coin));
    }

    if(mie_drv.user_attach) {
        mie_drv.user_attach(dev, mie_drv.user_attach_data);
    }

    dbglog(DBG_DEBUG, "mie: attached on port %c%d\n", 'A' + dev->port,
           dev->unit);
    return 0;
}

static void mie_port0_detach(maple_device_t *dev) {
    if(!dev || dev->drv != &mie_drv) {
        return;
    }

    dev->valid = false;

    if(mie_drv.user_detach) {
        mie_drv.user_detach(dev, mie_drv.user_detach_data);
    }
    if(mie_drv.detach) {
        mie_drv.detach(&mie_drv, dev);
    }

    dev->drv = NULL;
    dev->probe_mask = 0;
    dev->dev_mask = 0;
}

mie_port0_mode_t mie_port0_mode(void) {
    if(hardware_sys_mode(NULL) == HW_TYPE_RETAIL) {
        return MIE_PORT0_MAPLE;
    }

    return (mie_port0_mode_t)port0_mode;
}

/* Periodic IO state housekeeping: decay post-error backoff while idle,
   or abort a transaction that never completed. */
static void mie_io_tick(maple_device_t *dev) {
    mie_drv_state_t *ms;

    assert(dev);
    assert(dev->status);

    ms = dev->status;

    if(ms->io_state == MIE_IO_IDLE) {
        /* No in-flight transaction; stale timeout must not carry over. */
        ms->io_timeout = 0;
        /* After a protocol error mie_reply sets io_backoff; tick it down
           each period so mie_poll resumes input polling gradually. */
        if(ms->io_backoff > 0) {
            ms->io_backoff--;
        }
        return;
    }

    /* Transaction pending — mie_reply should advance io_state; if it does
       not within MIE_IO_TIMEOUT_MAX periodic ticks, force recovery. */
    if(++ms->io_timeout < MIE_IO_TIMEOUT_MAX) {
        return;
    }

    ms->io_timeout = 0;
    ms->io_state = MIE_IO_IDLE;
    mie_cmd_finish(MIE_DEF_CMD_ERR);
    mie_frame_reset(&dev->frame);
}

static void mie_bind_port0(void) {
    maple_device_t *dev;

    dev = maple_enum_dev(MIE_MAPLE_PORT, MIE_MAPLE_UNIT);
    if(!dev) {
        /* No Maple scan hit — install our static placeholder on port A. */
        maple_state.ports[MIE_MAPLE_PORT].units[MIE_MAPLE_UNIT] = &mie_static_dev;
        dev = &mie_static_dev;
    }

    mie_bind_device(dev);
}

/* Called every maple tick for each attached MIE device. */
static int mie_periodic_dev(maple_device_t *dev) {
    mie_drv_state_t *ms;

    assert(dev);
    assert(dev->status);

    ms = dev->status;

    /* Sync IO holds the bus: only drain in-flight fetch, no new polls. */
    if(mie_bus_suspend) {
        mie_io_tick(dev);

        if(ms->io_state == MIE_IO_NEED_FETCH) {
            return mie_queue_jvs_fetch(dev);
        }

        return -1;
    }

    mie_io_tick(dev);
    return mie_poll(dev);
}

static void mie_periodic(maple_driver_t *drv) {
    (void)drv;

    if(port0_mode != MIE_PORT0_JVS || !mie_jvs_ready) {
        return;
    }

    maple_driver_foreach(drv, mie_periodic_dev);
}

static int mie_bus_idle_check(void *data) {
    (void)data;

    return mie_static_state.io_state == MIE_IO_IDLE &&
           !mie_static_dev.frame.queued &&
           mie_cmd_frame.state == MAPLE_FRAME_VACANT &&
           !mie_cmd_frame.queued;
}

static bool mie_maple_idle_probe(void) {
    maple_response_t *resp;

    /* Idle probe succeeds only when the bridge is not mid-transaction. */
    resp = mie_cmd_resp(MIE_MAPLE_IDLE_PROBE, NULL, 0);
    return resp &&
           resp->response != MAPLE_RESPONSE_AGAIN &&
           resp->response != MAPLE_RESPONSE_NONE;
}

/* Wait until both frames are idle and bridge accepts a DEVINFO probe. */
bool mie_poll_idle(void) {
    uint64_t deadline = timer_ms_gettime64() + MIE_POLL_IDLE_TIMEOUT_MS;
    uint64_t now;
    unsigned long remain;

    for(;;) {
        now = timer_ms_gettime64();
        if(now >= deadline) {
            return false;
        }

        remain = (unsigned long)(deadline - now);

        if(thd_poll(mie_bus_idle_check, NULL, remain) <= 0) {
            return false;
        }

        if(mie_maple_idle_probe()) {
            return true;
        }
    }
}

maple_response_t *mie_jvs_fetch_retry(bool (*ready)(maple_response_t *)) {
    uint8_t fetch[4];
    maple_response_t *resp;
    int i;

    mie_build_jvs_fetch(fetch);

    /* Z80 fills the IOR buffer asynchronously; retry until data is ready. */
    for(i = 0; i < 80; i++) {
        if(i > 0) {
            thd_sleep(20);
        }

        resp = mie_cmd_resp(MIE_CMD_IO, fetch, 1);

        if(!resp || resp->data_len < 1
            || resp->response == MAPLE_RESPONSE_AGAIN
            || (uint8_t)resp->response != MIE_RESP_IO) {
            continue;
        }

        if(((uint8_t *)resp->data)[0] == MIE_IOR_JVS_EMPTY) {
            continue;
        }

        if(!ready || ready(resp)) {
            return resp;
        }
    }

    return NULL;
}

/* Synchronous JVS IO for init/reset; caller must hold mie_bus_acquire. */
bool mie_jvs_io_cmd(const uint8_t *req, int length) {
    maple_response_t *resp;
    uint8_t code;

    assert(req);
    assert(length > 0);

    mie_bus_acquire_scoped(bus);
    if(!bus) {
        return false;
    }

    if(!mie_poll_idle()) {
        return false;
    }

    resp = mie_cmd_resp(MIE_CMD_IO, (void *)req, length);
    if(!resp) {
        return false;
    }

    code = (uint8_t)resp->response;
    return code == MIE_RESP_IO;
}

static void mie_cmd_cb(maple_state_t *st, maple_frame_t *frm) {
    (void)st;
    maple_frame_unlock(frm);
    genwait_wake_all(frm);
}

/* Blocking Maple command on dedicated frame (init, eeprom, fw upload). */
maple_response_t *mie_cmd_resp(int cmd, void *send_buf, int length) {
    maple_frame_t *frm = &mie_cmd_frame;
    maple_response_t *resp;

    if(frm->queued && frm->state == MAPLE_FRAME_VACANT) {
        mie_frame_reset(frm);
    }

    if(maple_frame_lock(frm) < 0) {
        return NULL;
    }

    maple_frame_init(frm);
    ((uint32_t *)frm->recv_buf)[0] = 0xffffffff;
    frm->cmd = cmd;
    frm->dst_port = MIE_MAPLE_PORT;
    frm->dst_unit = MIE_MAPLE_UNIT;
    frm->length = length;
    if(length > 0 && send_buf) {
        memcpy(mie_io_tx_arr, send_buf, length * 4);
    }
    frm->send_buf = (uint32_t *)mie_io_tx_arr;
    frm->callback = mie_cmd_cb;
    maple_queue_frame(frm);

    if(!maple_state.dma_in_progress) {
        maple_queue_flush();
    }

    if(genwait_wait(frm, "mie_cmd", 1000) < 0) {
        mie_frame_reset(frm);
        return NULL;
    }

    resp = (maple_response_t *)frm->recv_buf;
    return resp;
}

static maple_response_t *mie_cmd(int cmd, void *send_buf, int length,
                                      int expect_resp) {
    maple_response_t *resp;

    resp = mie_cmd_resp(cmd, send_buf, length);
    if(!resp) {
        return NULL;
    }

    if((uint8_t)resp->response != (uint8_t)expect_resp) {
        return NULL;
    }

    return resp;
}

uint16_t mie_get_coin_meter(uint8_t slot) {
    if(slot >= MIE_JVS_COIN_SLOTS) {
        return 0;
    }

    return mie_static_state.jvs.coin[slot].count;
}

bool mie_read_coin_input(mie_jvs_coin_slot_t *out, int max_slots) {
    int slots;
    int i;

    if(!out || max_slots < 1 || !mie_z80_init_done) {
        return false;
    }

    slots = max_slots;
    if(slots > MIE_JVS_COIN_SLOTS) {
        slots = MIE_JVS_COIN_SLOTS;
    }

    for(i = 0; i < slots; i++) {
        out[i] = mie_static_state.jvs.coin[i];
    }

    return true;
}

bool mie_coin_decrease(uint8_t slot, uint16_t amount, bool block) {
    uint8_t req[12];

    if(!mie_z80_init_done || amount == 0) {
        return false;
    }

    mie_build_jvs_coin_dec(req, slot, amount);
    return mie_cmd_submit(req, MIE_COIN_CMD_TX_WORDS, block);
}

bool mie_coin_add(uint8_t slot, uint16_t amount, bool block) {
    uint8_t req[12];

    if(!mie_z80_init_done || amount == 0) {
        return false;
    }

    mie_build_jvs_coin_add(req, slot, amount);
    return mie_cmd_submit(req, MIE_COIN_CMD_TX_WORDS, block);
}

static void mie_jvs_output_mask_to_wire(uint32_t mask, uint8_t *wire,
                                        uint8_t wire_bytes) {
    uint8_t i;

    for(i = 0; i < wire_bytes; i++) {
        wire[i] = (uint8_t)((mask >> (24 - i * 8)) & GENMASK(7, 0));
    }
}

bool mie_jvs_set_outputs(uint32_t outputs, bool block) {
    uint8_t req[16];
    uint8_t wire[(MIE_JVS_OUTPUT_COUNT + 7) / 8];
    uint8_t wire_bytes;
    uint8_t tx_words;

    if(!mie_z80_init_done) {
        return false;
    }

    wire_bytes = mie_jvs_output_wire_bytes();
    mie_jvs_output_mask_to_wire(outputs, wire, wire_bytes);
    tx_words = mie_jvs_output_tx_words(wire_bytes);

    mie_build_jvs_output(req, wire, wire_bytes);
    if(!mie_cmd_submit(req, tx_words, block)) {
        return false;
    }

    mie_jvs_output_cache = outputs;
    return true;
}

bool mie_jvs_set_output(uint8_t index, bool on, bool block) {
    uint32_t bit;

    if(index >= MIE_JVS_OUTPUT_COUNT) {
        return false;
    }

    bit = MIE_JVS_OUTPUT_MASK(index);
    if(on) {
        return mie_jvs_set_outputs(mie_jvs_output_cache | bit, block);
    }

    return mie_jvs_set_outputs(mie_jvs_output_cache & ~bit, block);
}

uint32_t mie_jvs_get_outputs(void) {
    return mie_jvs_output_cache;
}

char *mie_get_id(char *dst) {
    maple_response_t *resp;

    assert(dst);

    mie_bus_acquire_scoped(bus);
    if(!bus) {
        dst[0] = '\0';
        return NULL;
    }

    resp = mie_cmd(MIE_CMD_GET_ID, NULL, 0, MIE_RESP_GET_ID);
    if(!resp) {
        dst[0] = '\0';
        return NULL;
    }

    if(mie_copy_id(resp, dst, MIE_ID_SIZE) < 1) {
        dst[0] = '\0';
        return NULL;
    }

    return dst;
}

static void mie_port0_set_mode(mie_port0_mode_t mode) {
    port0_mode = mode;
    maple_state.port0_mie = (mode == MIE_PORT0_JVS);
}

/* Called from maple scan: skip probe if a normal controller already on port A. */
void mie_init_scan(void) {
    if(!mie_drv_registered) {
        return;
    }

    if(port0_mode == MIE_PORT0_UNKNOWN) {
        if(maple_dev_valid(MIE_MAPLE_PORT, MIE_MAPLE_UNIT)) {
            mie_port0_set_mode(MIE_PORT0_MAPLE);
            return;
        }
    }

    if(port0_mode != MIE_PORT0_MAPLE) {
        mie_init_common(NULL);
    }
}

/* Detect JVS bridge on port A, bind driver, upload/activate Z80 firmware. */
static void mie_init_common(const char *fw_path) {
    maple_response_t *resp;
    char id[MIE_ID_SIZE];

    if(port0_mode == MIE_PORT0_MAPLE) {
        return;
    }

    /* Probe GET_ID: MIE present -> JVS mode, absent -> normal Maple on port A. */
    if(port0_mode == MIE_PORT0_UNKNOWN) {
        resp = mie_cmd(MIE_CMD_GET_ID, NULL, 0, MIE_RESP_GET_ID);

        if(!resp) {
            dbglog(DBG_INFO, "mie: no JVS bridge, port A in Maple mode\n");
            mie_port0_set_mode(MIE_PORT0_MAPLE);
            return;
        }

        if(mie_copy_id(resp, id, sizeof(id)) >= 1) {
            dbglog(DBG_INFO, "mie: JVS I/O %s\n", id);
        }
        mie_port0_set_mode(MIE_PORT0_JVS);
    }

    if(port0_mode != MIE_PORT0_JVS) {
        return;
    }

    mie_bind_port0();

    if(!mie_z80_init_done) {
        mie_poll_idle();

        /* Prefer existing Z80 code; upload fw_path only when probe fails. */
        if(fw_path && fw_path[0]) {
            if(mie_z80_init(fw_path, &mie_static_dev)) {
                mie_z80_init_done = true;
            }
        }
        else if(mie_z80_try_active(&mie_static_dev)) {
            mie_z80_init_done = true;
        }
    }

    if(mie_z80_init_done) {
        mie_scan_complete();
    }
}
/* Unblocks periodic input polling after Z80 firmware is verified. */
void mie_scan_complete(void) {
    mie_jvs_ready = port0_mode == MIE_PORT0_JVS;
}

bool mie_init_fw(const char *fw_path) {
    if(hardware_sys_mode(NULL) == HW_TYPE_RETAIL) {
        return false;
    }

    if(!fw_path || !fw_path[0]) {
        return false;
    }

    if(mie_z80_init_done) {
        return true;
    }

    if(!mie_drv_registered) {
        mie_init();
    }

    if(!mie_drv_registered) {
        return false;
    }

    mie_init_common(fw_path);

    return mie_z80_init_done;
}

void mie_init(void) {
    if(hardware_sys_mode(NULL) == HW_TYPE_RETAIL) {
        return;
    }

    if(mie_drv_registered) {
        return;
    }

    mie_set_cont_map(NULL);
    mie_analog_calib_reset();
    mie_callbacks_init();

    mie_jvs_ready = false;
    mie_z80_init_done = false;

    memset(&mie_cmd_frame, 0, sizeof(mie_cmd_frame));
    mie_cmd_frame.state = MAPLE_FRAME_VACANT;

    maple_driver_reg(&mie_drv);
    mie_drv_registered = true;
}

void mie_shutdown(void) {
    maple_device_t **mie_unit;

    if(!mie_drv_registered) {
        return;
    }

    mie_callbacks_shutdown();
    mie_port0_detach(&mie_static_dev);
    mie_unit = &maple_state.ports[MIE_MAPLE_PORT].units[MIE_MAPLE_UNIT];
    if(*mie_unit == &mie_static_dev) {
        *mie_unit = NULL;
    }
    mie_port0_set_mode(MIE_PORT0_UNKNOWN);
    mie_jvs_ready = false;
    mie_z80_init_done = false;
    mie_drv_registered = false;
    maple_driver_unreg(&mie_drv);
}
