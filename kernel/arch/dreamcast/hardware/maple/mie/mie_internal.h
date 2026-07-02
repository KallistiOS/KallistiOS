/* KallistiOS ##version##

   mie_internal.h
   Copyright (C) 2026 Ruslan Rostovtsev
*/

#ifndef MIE_INTERNAL_H
#define MIE_INTERNAL_H

#include <stdint.h>
#include <stdbool.h>

#include <dc/maple.h>
#include <dc/maple/mie.h>

/* Driver state for the single MIE bridge on port A. io_state drives the
   two-step Maple IO cycle: poll request (WAIT_ACK) -> JVS fetch
   (WAIT_FETCH) -> parse (IDLE). */
typedef struct mie_jvs_poll_cfg {
    uint8_t players;
    uint8_t sw_bytes;
    uint8_t coin_slots;
    uint8_t analog_channels;
    uint8_t driver_outputs;
} mie_jvs_poll_cfg_t;

typedef struct mie_drv_state {
    cont_state_t cont;       /* mapped Dreamcast controller state */
    mie_jvs_inputs_t jvs;    /* raw JVS inputs from last fetch */
    uint8_t io_state;        /* poll transaction phase (see mie.c enum) */
    uint8_t io_timeout;      /* periodic ticks before forced recovery */
    uint8_t io_backoff;      /* post-error delay before next poll */
    uint8_t input_baseline;  /* 0 until first valid frame (skip callbacks) */
    uint8_t coin_seeded;     /* bitmask: slot baseline initialized */
    uint8_t coin_dec_pending; /* bitmask: COINDEC queued for overflow slot */
} mie_drv_state_t;

void mie_build_jvs_coin_dec(uint8_t *buf, uint8_t slot, uint16_t amount);
void mie_build_jvs_coin_add(uint8_t *buf, uint8_t slot, uint16_t amount);
void mie_build_buttons_tx(uint8_t *buf);
void mie_build_jvs_reset(uint8_t *buf);
void mie_build_jvs_setaddr(uint8_t *buf);
void mie_build_jvs_func_check(uint8_t *buf);
void mie_build_jvs_fetch(uint8_t *buf);
uint8_t mie_jvs_poll_tx_words(void);
bool mie_jvs_poll_cfg_apply(const mie_jvs_poll_cfg_t *cfg);
uint8_t mie_jvs_driver_outputs(void);
uint8_t mie_jvs_output_wire_bytes(void);
uint8_t mie_jvs_output_tx_words(uint8_t data_bytes);
bool mie_parse_jvs_func_check(const uint8_t *data, int len,
                              mie_jvs_poll_cfg_t *cfg);
bool mie_ior_jvs_report_ready(const uint8_t *data, int len);
void mie_build_jvs_output(uint8_t *buf, const uint8_t *data, uint8_t bytes);

uint16_t mie_jvs_coin_meter(const mie_jvs_coin_slot_t *slot);
bool mie_jvs_coin_usable(const mie_jvs_coin_slot_t *slot);
uint16_t mie_jvs_coin_count(const mie_jvs_coin_slot_t *slot);
void mie_jvs_coin_slot_set(mie_jvs_coin_slot_t *slot, uint16_t raw,
                           uint16_t prev_count);
const uint8_t *mie_ior_resp_map(maple_response_t *resp, int *len_out);
bool mie_extract_coin(const uint8_t *data, int len, uint16_t *raw, int max_slots);
bool mie_extract_analog(const uint8_t *data, int len, uint16_t *analog);
bool mie_extract_panel(const uint8_t *data, int len, mie_jvs_panel_t *panel);
bool mie_extract_sw(const uint8_t *data, int len, uint8_t *sys, uint16_t *p1,
                    uint16_t *p2);
int mie_copy_id(maple_response_t *resp, char *buf, int buf_len);

void mie_callbacks_init(void);
void mie_callbacks_shutdown(void);
bool mie_apply_buttons(maple_response_t *resp, maple_device_t *dev);

void mie_analog_track(const mie_jvs_inputs_t *jvs);
int mie_analog_map_wheel(uint16_t raw);
int mie_analog_map_accel(uint16_t raw);
int mie_analog_map_brake(uint16_t raw);

bool mie_poll_idle(void);
bool mie_bus_acquire(void);  /* pause periodic poll, wait for bus drain */
void mie_bus_release(void);

static inline void mie_bus_scope_cleanup(bool *held) {
    if(*held) {
        mie_bus_release();
    }
}

#define mie_bus_acquire_scoped(name) \
    bool name __attribute__((cleanup(mie_bus_scope_cleanup))) = mie_bus_acquire()

maple_response_t *mie_cmd_resp(int cmd, void *send_buf, int length);
maple_response_t *mie_jvs_fetch_retry(bool (*ready)(maple_response_t *));
bool mie_jvs_io_cmd(const uint8_t *req, int length);
bool mie_read_coin_input(mie_jvs_coin_slot_t *out, int max_slots);

bool mie_z80_init(const char *fw_path, maple_device_t *dev);
bool mie_z80_try_active(maple_device_t *dev);
bool mie_jvs_function_check(void);

#endif
