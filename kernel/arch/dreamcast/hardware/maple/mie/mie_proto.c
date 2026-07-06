/* KallistiOS ##version##

   mie_proto.c
   Copyright (C) 2026 Ruslan Rostovtsev

   Protocol helpers: build Maple IO payloads and parse the Z80 IOR response
   buffer. JVS commands are nested inside IOR envelopes (REQUEST/FETCH/RESPONSE).
*/

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include <dc/maple.h>
#include <dc/maple/mie.h>

#include "mie_internal.h"

#define MIE_IOR_JVS_FETCH       0x15
#define MIE_IOR_JVS_RESPONSE    0x16
#define MIE_IOR_JVS_REQUEST     0x17
#define MIE_JVS_REQ_MAGIC       0x77

#define JVS_REPORT_OK 0x01

#define MIE_JVS_CMD_RESET   0xF0
#define MIE_JVS_CMD_SETADDR 0xF1
#define MIE_JVS_RESET_ARG   0xD9

#define JVS_CMD_SWINP       0x20
#define JVS_CMD_COININP     0x21
#define JVS_CMD_ANLINP      0x22
#define JVS_CMD_COINDEC     0x30
#define JVS_CMD_COINADD     0x35
#define JVS_CMD_OUTPUT      0x32
#define JVS_CMD_FUNC_CHECK  0x14

#define MIE_JVS_SW_BYTES_DEFAULT 2

/* Fixed offsets within the IOR JVS response buffer (see mie_ior_resp_ready). */
#define MIE_IOR_RESP_STATUS_OFF        18
#define MIE_IOR_RESP_STATUS_READY      0x80
#define MIE_IOR_RESP_JVS_REPORT_OFF    26
#define MIE_IOR_RESP_PANEL_OFF         6
#define MIE_IOR_RESP_PANEL_TEST_BIT    0x10
#define MIE_IOR_RESP_PANEL_SERVICE_BIT 0x20
#define MIE_IOR_RESP_SYS_OFF           27
#define MIE_IOR_RESP_P1_SW_OFF         28
#define MIE_IOR_RESP_P2_SW_OFF         30
#define MIE_IOR_RESP_COIN_OFF          32
#define MIE_IOR_RESP_ANALOG_OFF        38
#define MIE_IOR_RESP_MIN_LEN           54
#define MIE_IOR_RESP_SW_MIN_LEN        32

static mie_jvs_poll_cfg_t mie_jvs_poll_cfg = {
    MIE_JVS_PLAYER_SLOTS,
    MIE_JVS_SW_BYTES_DEFAULT,
    MIE_JVS_COIN_SLOTS,
    MIE_JVS_ANALOG_CHANNELS,
    0
};

static uint8_t mie_jvs_poll_bundle_len(void) {
    uint8_t len = 0;

    if(mie_jvs_poll_cfg.players > 0) {
        len = (uint8_t)(len + 3);
    }
    if(mie_jvs_poll_cfg.coin_slots > 0) {
        len = (uint8_t)(len + 2);
    }
    if(mie_jvs_poll_cfg.analog_channels > 0) {
        len = (uint8_t)(len + 2);
    }
    return len;
}

uint8_t mie_jvs_poll_tx_words(void) {
    return (uint8_t)((8 + mie_jvs_poll_bundle_len() + 3) / 4);
}

bool mie_jvs_poll_cfg_apply(const mie_jvs_poll_cfg_t *cfg) {
    mie_jvs_poll_cfg_t next;

    assert(cfg);

    next = *cfg;
    if(next.players == 0) {
        next.players = MIE_JVS_PLAYER_SLOTS;
    }
    if(next.players > MIE_JVS_PLAYER_SLOTS) {
        next.players = MIE_JVS_PLAYER_SLOTS;
    }
    if(next.sw_bytes == 0) {
        next.sw_bytes = MIE_JVS_SW_BYTES_DEFAULT;
    }
    if(next.coin_slots == 0) {
        next.coin_slots = MIE_JVS_COIN_SLOTS;
    }
    if(next.coin_slots > MIE_JVS_COIN_SLOTS) {
        next.coin_slots = MIE_JVS_COIN_SLOTS;
    }
    if(next.analog_channels == 0) {
        next.analog_channels = MIE_JVS_ANALOG_CHANNELS;
    }
    if(next.analog_channels > MIE_JVS_ANALOG_CHANNELS) {
        next.analog_channels = MIE_JVS_ANALOG_CHANNELS;
    }
    if(next.driver_outputs == 0) {
        next.driver_outputs = MIE_JVS_OUTPUT_COUNT;
    }
    if(next.driver_outputs > MIE_JVS_OUTPUT_COUNT) {
        next.driver_outputs = MIE_JVS_OUTPUT_COUNT;
    }

    mie_jvs_poll_cfg = next;
    return true;
}

uint8_t mie_jvs_driver_outputs(void) {
    if(mie_jvs_poll_cfg.driver_outputs == 0) {
        return MIE_JVS_OUTPUT_COUNT;
    }
    return mie_jvs_poll_cfg.driver_outputs;
}

uint8_t mie_jvs_output_wire_bytes(void) {
    uint8_t bytes;

    bytes = (uint8_t)((mie_jvs_driver_outputs() + 7) / 8);
    if(bytes > (MIE_JVS_OUTPUT_COUNT + 7) / 8) {
        bytes = (uint8_t)((MIE_JVS_OUTPUT_COUNT + 7) / 8);
    }
    if(bytes == 0) {
        bytes = 1;
    }
    return bytes;
}

uint8_t mie_jvs_output_tx_words(uint8_t data_bytes) {
    uint8_t jvs_len;

    jvs_len = (uint8_t)(2 + data_bytes);
    return (uint8_t)((8 + jvs_len + 3) / 4);
}

#define MIE_JVS_NODE      1
#define MIE_JVS_BROADCAST 0xFF

/* Wrap a raw JVS command in the MIE IOR request envelope (Maple IO payload). */
static uint8_t *mie_jvs_req_hdr(uint8_t *buf, uint8_t node, uint8_t jvs_len) {
    buf[0] = MIE_IOR_JVS_REQUEST;
    buf[1] = MIE_JVS_REQ_MAGIC;
    buf[6] = node;
    buf[7] = jvs_len;
    return buf + 8;
}

void mie_build_jvs_coin_cmd(uint8_t *buf, uint8_t jvs_cmd,
                                    uint8_t slot, uint16_t amount) {
    uint8_t *cmd;

    memset(buf, 0, 12);
    cmd = mie_jvs_req_hdr(buf, MIE_JVS_NODE, 4);
    cmd[0] = jvs_cmd;
    cmd[1] = slot + 1;
    cmd[2] = (uint8_t)(amount >> 8);
    cmd[3] = (uint8_t)(amount & 0xFF);
}

void mie_build_jvs_coin_dec(uint8_t *buf, uint8_t slot, uint16_t amount) {
    mie_build_jvs_coin_cmd(buf, JVS_CMD_COINDEC, slot, amount);
}

void mie_build_jvs_coin_add(uint8_t *buf, uint8_t slot, uint16_t amount) {
    mie_build_jvs_coin_cmd(buf, JVS_CMD_COINADD, slot, amount);
}

/* Single poll bundles switch, coin and analog reads for the periodic input loop. */
void mie_build_buttons_tx(uint8_t *buf) {
    uint8_t *cmd;
    uint8_t len = 0;

    memset(buf, 0, 16);
    cmd = mie_jvs_req_hdr(buf, MIE_JVS_NODE, 0);
    if(mie_jvs_poll_cfg.players > 0) {
        cmd[len++] = JVS_CMD_SWINP;
        cmd[len++] = mie_jvs_poll_cfg.players;
        cmd[len++] = mie_jvs_poll_cfg.sw_bytes;
    }
    if(mie_jvs_poll_cfg.coin_slots > 0) {
        cmd[len++] = JVS_CMD_COININP;
        cmd[len++] = mie_jvs_poll_cfg.coin_slots;
    }
    if(mie_jvs_poll_cfg.analog_channels > 0) {
        cmd[len++] = JVS_CMD_ANLINP;
        cmd[len++] = mie_jvs_poll_cfg.analog_channels;
    }
    buf[7] = len;
}

void mie_build_jvs_output(uint8_t *buf, const uint8_t *data, uint8_t bytes) {
    uint8_t *cmd;
    uint8_t i;

    memset(buf, 0, 12);
    cmd = mie_jvs_req_hdr(buf, MIE_JVS_NODE, (uint8_t)(2 + bytes));
    cmd[0] = JVS_CMD_OUTPUT;
    cmd[1] = bytes;
    for(i = 0; i < bytes; i++) {
        cmd[2 + i] = data[i];
    }
}

void mie_build_jvs_reset(uint8_t *buf) {
    uint8_t *cmd;

    memset(buf, 0, 12);
    /* Broadcast reset — all JVS nodes must see this before setaddr. */
    cmd = mie_jvs_req_hdr(buf, MIE_JVS_BROADCAST, 2);
    cmd[0] = MIE_JVS_CMD_RESET;
    cmd[1] = MIE_JVS_RESET_ARG;
}

void mie_build_jvs_setaddr(uint8_t *buf) {
    uint8_t *cmd;

    memset(buf, 0, 12);
    /* Assign node 1 after broadcast reset (retried until ack in mie_fw.c). */
    cmd = mie_jvs_req_hdr(buf, MIE_JVS_BROADCAST, 2);
    cmd[0] = MIE_JVS_CMD_SETADDR;
    cmd[1] = MIE_JVS_NODE;
}

void mie_build_jvs_func_check(uint8_t *buf) {
    uint8_t *cmd;

    memset(buf, 0, 12);
    cmd = mie_jvs_req_hdr(buf, MIE_JVS_NODE, 1);
    cmd[0] = JVS_CMD_FUNC_CHECK;
}

void mie_build_jvs_fetch(uint8_t *buf) {
    /* Second step of the poll cycle — read Z80-assembled JVS result. */
    buf[0] = MIE_IOR_JVS_FETCH;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
}

static inline uint16_t mie_read_jvs_u16(const uint8_t *p) {
    return ((uint16_t)p[0] << 8) | p[1];
}

/* IOR fetch buffer: status byte + JVS report must be OK before parsing fields. */
static bool mie_ior_resp_ready(const uint8_t *data, int len) {
    if(len < MIE_IOR_RESP_MIN_LEN || data[0] != MIE_IOR_JVS_RESPONSE) {
        return false;
    }

    if((data[MIE_IOR_RESP_STATUS_OFF] & MIE_IOR_RESP_STATUS_READY) == 0) {
        return false;
    }

    if(data[MIE_IOR_RESP_JVS_REPORT_OFF] != JVS_REPORT_OK) {
        return false;
    }

    return true;
}

bool mie_ior_jvs_report_ready(const uint8_t *data, int len) {
    if(len < MIE_IOR_RESP_JVS_REPORT_OFF + 2 ||
            data[0] != MIE_IOR_JVS_RESPONSE) {
        return false;
    }

    return data[MIE_IOR_RESP_JVS_REPORT_OFF] == JVS_REPORT_OK;
}

bool mie_parse_jvs_func_check(const uint8_t *data, int len,
                              mie_jvs_poll_cfg_t *cfg) {
    const uint8_t *p;
    const uint8_t *end;
    uint8_t type;
    uint8_t p1;
    uint8_t p2;
    bool found = false;

    assert(data);
    assert(cfg);

    if(!mie_ior_jvs_report_ready(data, len)) {
        return false;
    }

    memset(cfg, 0, sizeof(*cfg));
    p = data + MIE_IOR_RESP_JVS_REPORT_OFF + 1;
    end = data + len;

    while(p + 4 <= end && p[0] != 0) {
        type = p[0];
        p1 = p[1];
        p2 = p[2];
        p += 4;

        switch(type) {
        case 0x01:
            cfg->players = p1;
            if(p2 == 0) {
                cfg->sw_bytes = 0;
            }
            else {
                cfg->sw_bytes = (uint8_t)(((p2 - 1) / 8) + 1);
            }
            found = true;
            break;
        case 0x02:
            cfg->coin_slots = p1;
            found = true;
            break;
        case 0x03:
            cfg->analog_channels = p1;
            found = true;
            break;
        case 0x12:
            cfg->driver_outputs = p1;
            found = true;
            break;
        default:
            break;
        }
    }

    return found;
}

const uint8_t *mie_ior_resp_map(maple_response_t *resp, int *len_out) {
    const uint8_t *data;
    int len;

    if(!resp || resp->data_len < 2) {
        return NULL;
    }

    data = resp->data;
    len = (int)resp->data_len * 4;

    if(!mie_ior_resp_ready(data, len)) {
        return NULL;
    }

    if(len_out) {
        *len_out = len;
    }

    return data;
}

uint16_t mie_jvs_coin_meter(const mie_jvs_coin_slot_t *slot) {
    assert(slot);

    return slot->raw & (uint16_t)~MIE_JVS_COIN_BUSY;
}

bool mie_jvs_coin_usable(const mie_jvs_coin_slot_t *slot) {
    assert(slot);

    return (slot->raw & MIE_JVS_COIN_BUSY) == 0;
}

uint16_t mie_jvs_coin_count(const mie_jvs_coin_slot_t *slot) {
    assert(slot);

    if(!mie_jvs_coin_usable(slot)) {
        return slot->count;
    }

    return mie_jvs_coin_meter(slot);
}

void mie_jvs_coin_slot_set(mie_jvs_coin_slot_t *slot, uint16_t raw,
                           uint16_t prev_count) {
    assert(slot);

    slot->raw = raw;

    if(mie_jvs_coin_usable(slot)) {
        slot->count = mie_jvs_coin_meter(slot);
    }
    else {
        slot->count = prev_count;
    }
}

bool mie_extract_coin(const uint8_t *data, int len, uint16_t *raw, int max_slots) {
    static const uint8_t ior_off[] = {2, 0, 4};
    const uint8_t *base;
    int i;
    int slots;

    assert(data);
    assert(raw);
    assert(max_slots > 0);

    base = data + MIE_IOR_RESP_COIN_OFF;
    slots = max_slots;
    if(slots > MIE_JVS_COIN_SLOTS) {
        slots = MIE_JVS_COIN_SLOTS;
    }
    if(slots > (int)(sizeof(ior_off) / sizeof(ior_off[0]))) {
        slots = (int)(sizeof(ior_off) / sizeof(ior_off[0]));
    }

    for(i = 0; i < slots; i++) {
        if(base + ior_off[i] + 2 > data + len ||
                base + ior_off[i] + 2 > data + MIE_IOR_RESP_ANALOG_OFF) {
            return i > 0;
        }
        raw[i] = mie_read_jvs_u16(base + ior_off[i]);
    }

    return true;
}

bool mie_extract_analog(const uint8_t *data, int len, uint16_t *analog) {
    const uint8_t *p;
    int i;

    assert(data);
    assert(analog);

    p = data + MIE_IOR_RESP_ANALOG_OFF;
    if(p + MIE_JVS_ANALOG_CHANNELS * 2 > data + len) {
        return false;
    }

    for(i = 0; i < MIE_JVS_ANALOG_CHANNELS; i++) {
        analog[i] = mie_read_jvs_u16(p + i * 2);
    }
    return true;
}

bool mie_extract_panel(const uint8_t *data, int len, mie_jvs_panel_t *panel) {
    uint8_t b;

    assert(data);
    assert(panel);

    panel->dip.raw = 0;
    panel->psw.raw = 0;

    if(len < MIE_IOR_RESP_PANEL_OFF + 1) {
        return false;
    }

    b = data[MIE_IOR_RESP_PANEL_OFF];
    /* DIP switches and panel buttons are active-low in the IOR buffer. */
    panel->dip.raw = (uint8_t)(~b & GENMASK(MIE_JVS_PANEL_DIP_COUNT - 1, 0));
    if((b & MIE_IOR_RESP_PANEL_TEST_BIT) == 0) {
        panel->psw.test = 1;
    }
    if((b & MIE_IOR_RESP_PANEL_SERVICE_BIT) == 0) {
        panel->psw.service = 1;
    }
    return true;
}

bool mie_extract_sw(const uint8_t *data, int len, uint8_t *sys, uint16_t *p1,
                    uint16_t *p2) {
    assert(data);
    assert(p1);
    assert(p2);

    if(len < MIE_IOR_RESP_SW_MIN_LEN) {
        return false;
    }

    if(sys) {
        *sys = data[MIE_IOR_RESP_SYS_OFF];
    }

    *p1 = mie_read_jvs_u16(data + MIE_IOR_RESP_P1_SW_OFF);
    *p2 = mie_read_jvs_u16(data + MIE_IOR_RESP_P2_SW_OFF);
    return true;
}

int mie_copy_id(maple_response_t *resp, char *buf, int buf_len) {
    int out_len = 0;
    int i;
    uint32_t part_len;
    uint32_t copy_len;

    assert(resp);
    assert(buf);
    assert(buf_len > 0);

    /* GET_ID may span two chained maple response blocks. */
    for(i = 0; i < 2; i++) {
        part_len = resp->data_len * 4;

        if(!part_len) {
            break;
        }

        copy_len = part_len;
        if(out_len + (int)copy_len >= buf_len) {
            copy_len = (uint32_t)(buf_len - out_len - 1);
        }

        memcpy(buf + out_len, resp->data, copy_len);
        out_len += (int)copy_len;
        resp = (maple_response_t *)(&resp->data[part_len]);
    }

    buf[out_len] = '\0';
    return out_len;
}
