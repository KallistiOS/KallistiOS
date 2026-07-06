/* KallistiOS ##version##

   mie_input.c
   Copyright (C) 2026 Ruslan Rostovtsev

   Input path: IOR fetch payload -> JVS state -> optional cont_state mapping.
   Callbacks run in a shared worker thread on rising-edge button/JVS input matches.
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/queue.h>

#include <dc/maple/mie.h>

#include "mie_internal.h"

#include <kos/mutex.h>
#include <kos/worker_thread.h>

#ifndef MIE_CALLBACK_THD_STACK_SIZE
#define MIE_CALLBACK_THD_STACK_SIZE (8 * 1024)
#endif

typedef enum mie_cb_type {
    MIE_CB_CONT,
    MIE_CB_JVS
} mie_cb_type_t;

typedef struct mie_callback_params {
    mie_cb_type_t type;
    bool cb_pending;
    union {
        struct {
            mie_btn_callback_t cb;
            uint32_t match;
            uint32_t cur;
        } cont;
        struct {
            mie_jvs_callback_t cb;
            mie_jvs_input_t input;
            uint32_t mask;
            mie_jvs_input_t cur_input;
            uint32_t cur_mask;
        } jvs;
    } u;
    TAILQ_ENTRY(mie_callback_params) listent;
    STAILQ_ENTRY(mie_callback_params) pending;
} mie_callback_params_t;

TAILQ_HEAD(mie_callback_list, mie_callback_params);
struct mie_callback_list cbs = TAILQ_HEAD_INITIALIZER(cbs);

STAILQ_HEAD(mie_cb_pending_list, mie_callback_params);
static struct mie_cb_pending_list mie_cb_pending =
    STAILQ_HEAD_INITIALIZER(mie_cb_pending);

static kthread_worker_t *mie_cb_worker;

static void mie_default_cont_map(const mie_jvs_inputs_t *jvs,
                                  cont_state_t *cont);

static mutex_t cbs_mtx = MUTEX_INITIALIZER;
static mie_cont_map_fn_t cont_map_fn = mie_default_cont_map;

static bool mie_callback_same_key(const mie_callback_params_t *a,
                                     const mie_callback_params_t *b) {
    if(a->type != b->type) {
        return false;
    }

    if(a->type == MIE_CB_CONT) {
        return a->u.cont.match == b->u.cont.match;
    }

    return a->u.jvs.input == b->u.jvs.input &&
           a->u.jvs.mask == b->u.jvs.mask;
}

static bool mie_callback_same_cb(const mie_callback_params_t *a,
                                    const mie_callback_params_t *b) {
    if(a->type == MIE_CB_CONT) {
        return a->u.cont.cb == b->u.cont.cb;
    }

    return a->u.jvs.cb == b->u.jvs.cb;
}

static void mie_callbacks_invoke(int baseline, uint32_t prev_btns,
                                  uint32_t cur_btns,
                                  const mie_jvs_inputs_t *prev_jvs,
                                  const mie_jvs_inputs_t *cur_jvs);

static void mie_coin_meter_apply(mie_drv_state_t *state,
                                  const mie_jvs_inputs_t *prev_jvs,
                                  const uint16_t *raws, int raw_count);

static void mie_cb_worker_fn(void *d) {
    STAILQ_HEAD(, mie_callback_params) batch;
    mie_callback_params_t *c, *n;

    (void)d;

    STAILQ_INIT(&batch);

    mutex_lock(&cbs_mtx);
    STAILQ_CONCAT(&batch, &mie_cb_pending);
    mutex_unlock(&cbs_mtx);

    STAILQ_FOREACH_SAFE(c, &batch, pending, n) {
        c->cb_pending = false;

        if(c->type == MIE_CB_CONT) {
            c->u.cont.cb(c->u.cont.cur);
        }
        else {
            c->u.jvs.cb(c->u.jvs.cur_input, c->u.jvs.cur_mask);
        }
    }
}

void mie_callbacks_init(void) {
    const kthread_attr_t thd_attr = {
        .stack_size = MIE_CALLBACK_THD_STACK_SIZE,
        .prio = PRIO_DEFAULT,
        .label = "mie_callback"
    };

    TAILQ_INIT(&cbs);
    STAILQ_INIT(&mie_cb_pending);
    mie_cb_worker = thd_worker_create_ex(&thd_attr, mie_cb_worker_fn, NULL);
}

static void mie_callback_del(mie_callback_params_t *params) {
    mie_callback_params_t *c, *n;

    mutex_lock_scoped(&cbs_mtx);

    TAILQ_FOREACH_SAFE(c, &cbs, listent, n) {
        if((params == NULL) || mie_callback_same_key(params, c)) {
            if((params == NULL) ||
                    ((params->type == MIE_CB_CONT && params->u.cont.cb == NULL) ||
                     (params->type == MIE_CB_JVS && params->u.jvs.cb == NULL) ||
                     mie_callback_same_cb(params, c))) {
                if(c->cb_pending) {
                    STAILQ_REMOVE(&mie_cb_pending, c, mie_callback_params,
                                  pending);
                    c->cb_pending = false;
                }
                TAILQ_REMOVE(&cbs, c, listent);
                free(c);
            }
        }
    }
}

void mie_callbacks_shutdown(void) {
    mie_callback_del(NULL);

    if(mie_cb_worker) {
        thd_worker_destroy(mie_cb_worker);
        mie_cb_worker = NULL;
    }
}

/* Parse IOR fetch payload, update JVS/cont state, fire edge-triggered callbacks. */
bool mie_apply_buttons(maple_response_t *resp, maple_device_t *dev) {
    mie_drv_state_t *state;
    mie_jvs_inputs_t prev_jvs;
    uint32_t prev_buttons;
    int prev_ltrig;
    int prev_rtrig;
    uint16_t p1, p2;
    uint8_t sys;
    uint16_t analog[MIE_JVS_ANALOG_CHANNELS];
    mie_jvs_panel_t panel;
    uint16_t coin_raw[MIE_JVS_COIN_SLOTS];
    bool has_analog;
    bool has_coin;
    bool has_panel;
    int baseline;
    const uint8_t *data;
    int len;

    assert(resp);
    assert(dev);
    assert(dev->status);

    state = dev->status;

    data = mie_ior_resp_map(resp, &len);
    if(!data) {
        return false;
    }

    if(!mie_extract_sw(data, len, &sys, &p1, &p2)) {
        return false;
    }

    /* Snapshot previous state for edge detection and analog hold-over. */
    prev_buttons = state->cont.buttons;
    prev_ltrig = state->cont.ltrig;
    prev_rtrig = state->cont.rtrig;
    prev_jvs = state->jvs;
    baseline = state->input_baseline;

    has_analog = mie_extract_analog(data, len, analog);
    has_coin = mie_extract_coin(data, len, coin_raw, MIE_JVS_COIN_SLOTS);
    has_panel = mie_extract_panel(data, len, &panel);

    state->jvs.system.raw = sys;
    state->jvs.p1.raw = p1;
    state->jvs.p2.raw = p2;
    state->jvs.coin_pulse = 0;
    state->jvs.coin_fault = 0;

    if(has_panel) {
        state->jvs.panel = panel;
    }

    if(has_coin) {
        mie_coin_meter_apply(state, &prev_jvs, coin_raw, MIE_JVS_COIN_SLOTS);
    }

    if(has_analog) {
        memcpy(state->jvs.analog, analog,
               sizeof(uint16_t) * MIE_JVS_ANALOG_CHANNELS);
        mie_analog_track(&state->jvs);
    }

    /* Map JVS inputs to cont_state (default or user-provided mapper). */
    cont_map_fn(&state->jvs, &state->cont);

    /* Bundled poll may omit analog; keep last trigger values in that case. */
    if(!has_analog) {
        state->cont.joyx = 0;
        state->cont.ltrig = prev_ltrig;
        state->cont.rtrig = prev_rtrig;
    }

    mie_callbacks_invoke(baseline, prev_buttons, state->cont.buttons,
                         &prev_jvs, &state->jvs);

    state->jvs.coin_pulse = 0;

    /* Skip callbacks on the very first frame to avoid spurious edges. */
    if(!state->input_baseline) {
        state->input_baseline = 1;
    }

    dev->valid = true;
    return true;
}

/* Track coin meter deltas; first sample seeds baseline without pulse events. */
static void mie_coin_meter_apply(mie_drv_state_t *state,
                                  const mie_jvs_inputs_t *prev_jvs,
                                  const uint16_t *raws, int raw_count) {
    uint8_t slot;
    mie_jvs_coin_slot_t cur;
    uint16_t read_count;
    uint16_t prev;
    uint8_t bit;

    if(raw_count > MIE_JVS_COIN_SLOTS) {
        raw_count = MIE_JVS_COIN_SLOTS;
    }

    for(slot = 0; slot < (uint8_t)raw_count; slot++) {

        prev = prev_jvs->coin[slot].count;
        bit = (uint8_t)BIT(slot);

        cur.raw = raws[slot];

        if(!mie_jvs_coin_usable(&cur)) {
            state->jvs.coin[slot].raw = cur.raw;
            state->jvs.coin_fault |= bit;
            if(!(state->coin_seeded & bit)) {
                state->jvs.coin[slot].count = prev;
                state->coin_seeded |= bit;
            }
            continue;
        }

        read_count = mie_jvs_coin_meter(&cur);

        if(!(state->coin_seeded & bit)) {
            mie_jvs_coin_slot_set(&state->jvs.coin[slot], raws[slot], prev);
            state->coin_seeded |= bit;
            if(!state->input_baseline) {
                continue;
            }
            state->jvs.coin[slot].count = prev;
        }

        if(read_count > MIE_JVS_COIN_DEC_LIMIT) {
            if((state->coin_dec_pending & bit) == 0) {
                mie_coin_decrease(slot, read_count, false);
                state->coin_dec_pending |= bit;
            }
        }
        else {
            state->coin_dec_pending &= (uint8_t)~bit;
        }

        if(read_count == prev) {
            state->jvs.coin[slot].raw = cur.raw;
            state->jvs.coin[slot].count = read_count;
            continue;
        }

        mie_jvs_coin_slot_set(&state->jvs.coin[slot], raws[slot], prev);

        if(state->input_baseline && read_count > prev) {
            state->jvs.coin_pulse |= bit;
        }
    }
}

static int mie_callback_register(mie_callback_params_t *params) {
    if(!mie_cb_worker) {
        return -1;
    }

    params->cb_pending = false;

    mutex_lock_scoped(&cbs_mtx);
    TAILQ_INSERT_TAIL(&cbs, params, listent);

    return 0;
}

int mie_btn_callback(uint32_t btns, mie_btn_callback_t cb) {
    mie_callback_params_t *params;

    params = (mie_callback_params_t *)malloc(sizeof(mie_callback_params_t));

    if(!params) {
        return -1;
    }

    params->type = MIE_CB_CONT;
    params->u.cont.match = btns;
    params->u.cont.cb = cb;

    if(cb == NULL) {
        mie_callback_del(params);
        free(params);
        return 0;
    }

    if(mie_callback_register(params) < 0) {
        free(params);
        return -1;
    }

    return 0;
}

int mie_jvs_callback(mie_jvs_input_t input, uint32_t mask,
                     mie_jvs_callback_t cb) {
    mie_callback_params_t *params;

    params = (mie_callback_params_t *)malloc(sizeof(mie_callback_params_t));

    if(!params) {
        return -1;
    }

    params->type = MIE_CB_JVS;
    params->u.jvs.input = input;
    params->u.jvs.mask = mask;
    params->u.jvs.cb = cb;

    if(cb == NULL) {
        mie_callback_del(params);
        free(params);
        return 0;
    }

    if(mie_callback_register(params) < 0) {
        free(params);
        return -1;
    }

    return 0;
}

static uint32_t mie_jvs_input_val(mie_jvs_input_t input,
                                   const mie_jvs_inputs_t *jvs) {
    switch(input) {
        case MIE_JVS_IN_SYSTEM:
            return jvs->system.raw;
        case MIE_JVS_IN_P1:
            return jvs->p1.raw;
        case MIE_JVS_IN_P2:
            return jvs->p2.raw;
        case MIE_JVS_IN_PANEL_DIP:
            return jvs->panel.dip.raw;
        case MIE_JVS_IN_PANEL_PSW:
            return jvs->panel.psw.raw;
        case MIE_JVS_IN_COIN1:
            return (jvs->coin_pulse & BIT(0)) ? MIE_JVS_COIN_INSERT_BIT : 0;
        case MIE_JVS_IN_COIN2:
            return (jvs->coin_pulse & BIT(1)) ? MIE_JVS_COIN_INSERT_BIT : 0;
        default:
            return 0;
    }
}

/* Queue callbacks for the shared worker on rising-edge matches. */
static void mie_callbacks_invoke(int baseline, uint32_t prev_btns,
                                  uint32_t cur_btns,
                                  const mie_jvs_inputs_t *prev_jvs,
                                  const mie_jvs_inputs_t *cur_jvs) {
    mie_callback_params_t *c;
    uint32_t prev_val;
    uint32_t cur_val;
    bool wake_worker = false;

    if(!baseline || !mie_cb_worker) {
        return;
    }

    if(mutex_lock_irqsafe(&cbs_mtx)) {
        return;
    }

    TAILQ_FOREACH(c, &cbs, listent) {
        if(c->type == MIE_CB_CONT) {
            if((cur_btns & c->u.cont.match) == c->u.cont.match &&
                    (prev_btns & c->u.cont.match) != c->u.cont.match) {
                c->u.cont.cur = cur_btns;
                if(!c->cb_pending) {
                    c->cb_pending = true;
                    STAILQ_INSERT_TAIL(&mie_cb_pending, c, pending);
                    wake_worker = true;
                }
            }
        }
        else {
            prev_val = mie_jvs_input_val(c->u.jvs.input, prev_jvs);
            cur_val = mie_jvs_input_val(c->u.jvs.input, cur_jvs);
            if((cur_val & c->u.jvs.mask) == c->u.jvs.mask &&
                    (prev_val & c->u.jvs.mask) != c->u.jvs.mask) {
                c->u.jvs.cur_mask = cur_val & c->u.jvs.mask;
                c->u.jvs.cur_input = c->u.jvs.input;
                if(!c->cb_pending) {
                    c->cb_pending = true;
                    STAILQ_INSERT_TAIL(&mie_cb_pending, c, pending);
                    wake_worker = true;
                }
            }
        }
    }

    mutex_unlock(&cbs_mtx);

    if(wake_worker) {
        thd_worker_wakeup(mie_cb_worker);
    }
}

static bool mie_default_analog_unused(const mie_jvs_inputs_t *jvs) {
    uint16_t w;
    uint16_t p;
    uint16_t diff;

    /* Wheel and accel near each other means no real steering hardware. */
    w = (uint16_t)(jvs->wheel & MIE_JVS_ANALOG_RAW_MASK);
    p = (uint16_t)(jvs->accel & MIE_JVS_ANALOG_RAW_MASK);
    if(w > p) {
        diff = w - p;
    }
    else {
        diff = p - w;
    }

    return diff <= 0x500;
}

/* Default JVS arcade inputs -> Dreamcast controller mapping. */
static void mie_default_cont_map(const mie_jvs_inputs_t *jvs,
                                  cont_state_t *cont) {
    uint32_t buttons = 0;

    assert(jvs);
    assert(cont);

    if(jvs->p1.start || jvs->p2.start) {
        buttons |= CONT_START;
    }
    if(jvs->p1.left || jvs->p2.left) {
        buttons |= CONT_DPAD_LEFT;
    }
    if(jvs->p1.right || jvs->p2.right) {
        buttons |= CONT_DPAD_RIGHT;
    }
    if(jvs->p1.up || jvs->p2.up) {
        buttons |= CONT_DPAD_UP;
    }
    if(jvs->p1.down || jvs->p2.down) {
        buttons |= CONT_DPAD_DOWN;
    }
    if(jvs->p1.sw16 || jvs->p2.sw16) {
        buttons |= CONT_A;
    }
    if(jvs->p1.sw2 || jvs->p2.sw2) {
        buttons |= CONT_B;
    }
    if(jvs->p1.sw1 || jvs->p2.sw1) {
        buttons |= CONT_X;
    }
    if(jvs->p1.sw14 || jvs->p2.sw14) {
        buttons |= CONT_Y;
    }
    if(jvs->p1.service || jvs->p2.service) {
        buttons |= CONT_Z;
    }
    if(jvs->panel.psw.test) {
        buttons |= CONT_C;
    }
    if(jvs->panel.psw.service) {
        buttons |= CONT_D;
    }

    cont->buttons = buttons;
    cont->joyy = 0;
    cont->joy2x = 0;
    cont->joy2y = 0;
    if(mie_default_analog_unused(jvs)) {
        cont->joyx = 0;
        cont->ltrig = 0;
        cont->rtrig = 0;
    }
    else {
        /* Wheel/accel/brake when JVS reports distinct analog channels. */
        cont->joyx = mie_analog_map_wheel(jvs->wheel);
        cont->rtrig = mie_analog_map_accel(jvs->accel);
        cont->ltrig = mie_analog_map_brake(jvs->brake);
    }
}

void mie_set_cont_map(mie_cont_map_fn_t fn) {
    cont_map_fn = fn ? fn : mie_default_cont_map;
}
