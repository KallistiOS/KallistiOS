/* KallistiOS ##version##

   libppp/pap.c
   Copyright (C) 2007, 2014 Lawrence Sebald
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <ppp/ppp.h>

#include <arch/timer.h>

#include "ppp_internal.h"

static ppp_state_t *ppp_state;
static uint8_t pap_id = 0;
static uint64_t next_resend;
static int resend_cnt;

#define PAP_AUTHENTICATE_REQ    1
#define PAP_AUTHENTICATE_ACK    2
#define PAP_AUTHENTICATE_NAK    3

typedef struct pap_packet {
    uint8_t code;
    uint8_t id;
    uint16_t len;
    uint8_t data[];
} __packed pap_pkt_t;

static int pap_send_auth_req(ppp_protocol_t *self, int resend) {
    int nl = strlen(ppp_state->username);
    int pl = strlen(ppp_state->passwd);
    uint16_t len = 6 + nl + pl;
    uint8_t buf[len];
    int i = 0, j;
    pap_pkt_t *pkt = (pap_pkt_t *)buf;

    (void)self;

    /* Fill in the header. */
    pkt->code = PAP_AUTHENTICATE_REQ;

    if(resend)
        pkt->id = pap_id;
    else
        pkt->id = ++pap_id;

    pkt->len = htons(len);

    /* Fill in the rest of the packet. */
    pkt->data[i++] = nl;

    for(j = 0; j < nl; ++j) {
        pkt->data[i++] = ppp_state->username[j];
    }

    pkt->data[i++] = pl;

    for(j = 0; j < pl; ++j) {
        pkt->data[i++] = ppp_state->passwd[j];
    }

    /* Set the resend timer for 3 seconds. */
    next_resend = timer_ms_gettime64() + 3000;

    if(!resend)
        resend_cnt = 10;

    return ppp_send(buf, len, PPP_PROTOCOL_PAP);
}

/* PPP Protocol implementation functions. */
static int pap_shutdown(ppp_protocol_t *self) {
    return ppp_del_protocol(self);
}

static int pap_input(ppp_protocol_t *self, const uint8_t *buf, size_t len) {
    const pap_pkt_t *pkt = (const pap_pkt_t *)buf;

    (void)self;

    /* Sanity check. */
    if(len < sizeof(pap_pkt_t) || len != ntohs(pkt->len)) {
        /* Drop bad packet. Should probably log this at some point. */
        return -1;
    }

    switch(pkt->code) {
        case PAP_AUTHENTICATE_REQ:
            DBG("pap: dropping spurious auth request\n");
            return 0;

        case PAP_AUTHENTICATE_ACK:
            /* If we are still in the authenticate phase (which we should be),
               move onto the network phase once we get an authenticate ack. */
            resend_cnt = -1;
            if(ppp_state->phase == PPP_PHASE_AUTHENTICATE)
                _ppp_enter_phase(PPP_PHASE_NETWORK);
            return 0;

        case PAP_AUTHENTICATE_NAK:
            /* If we get an authenticate nak, then the user has given us an
               invalid username/password pair, most likely. Give up on the
               connection. */
            resend_cnt = -1;
            _ppp_enter_phase(PPP_PHASE_DEAD);
            return 0;

        default:
            DBG("pap: ignoring unknown code: %d\n", pkt->code);
            return 0;
    }
}

static void pap_enter_phase(ppp_protocol_t *self, int oldp, int newp) {
    (void)self;
    (void)oldp;

    /* We only care about when we're entering the authenticate phase. */
    if(newp == PPP_PHASE_AUTHENTICATE &&
       ppp_state->auth_proto == PPP_PROTOCOL_PAP) {
        /* If we don't have a username/password, we're in trouble! */
        if(!ppp_state->username || !ppp_state->passwd) {
            return;
        }

        pap_send_auth_req(self, 0);
    }
}

static void pap_check_timeouts(ppp_protocol_t *self, uint64_t tm) {
    (void)self;

    if(resend_cnt >= 0 && tm >= next_resend) {
        if(!resend_cnt) {
            /* We've got no response, assume the other side is dead. */
            resend_cnt = -1;
            _ppp_enter_phase(PPP_PHASE_DEAD);
        }
        else {
            pap_send_auth_req(self, 1);
            --resend_cnt;
        }
    }
}

static ppp_protocol_t pap_proto = {
    PPP_PROTO_ENTRY_INIT,
    "pap",
    PPP_PROTOCOL_PAP,
    NULL,                   /* privdata */
    NULL,                   /* init */
    &pap_shutdown,
    &pap_input,
    &pap_enter_phase,
    &pap_check_timeouts
};

int _ppp_pap_init(ppp_state_t *st) {
    ppp_state = st;

    resend_cnt = -1;
    return ppp_add_protocol(&pap_proto);
}
