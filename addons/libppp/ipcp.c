/* KallistiOS ##version##

   libppp/ipcp.c
   Copyright (C) 2007, 2014 Lawrence Sebald
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <ppp/ppp.h>

#include <kos/net.h>

#include <arch/timer.h>

#include "ppp_internal.h"
#include "net_ipv4.h"

struct ipcp_state_s {
    int state;
    uint8_t last_conf;
    uint8_t last_term;
    uint8_t last_coderej;

    ppp_state_t *ppp_state;
    netif_t *nif;

    uint64_t next_resend;
    uint16_t resend_cnt;
    int (*resend_pkt)(ppp_protocol_t *, int);
    void (*resend_timeout)(ppp_protocol_t *);
} ipcp_state;

/* IPCP configuration options. */
#define IPCP_CONFIGURE_IP_ADDRESSES   1
#define IPCP_CONFIGURE_IP_COMPRESSION 2
#define IPCP_CONFIGURE_IP_ADDRESS     3
#define IPCP_CONFIGURE_PRIMARY_DNS    129
#define IPCP_CONFIGURE_PRIMARY_NBNS   130
#define IPCP_CONFIGURE_SECONDARY_DNS  131
#define IPCP_CONFIGURE_SECONDARY_NBNS 132

static void ipcp_cfg_timeout(ppp_protocol_t *self) {
    (void)self;

    /* The link is probably dead here... */
    ipcp_state.resend_pkt = NULL;
    ipcp_state.resend_timeout = NULL;

    _ppp_enter_phase(PPP_PHASE_DEAD);
}

static int ipcp_send_client_cfg(ppp_protocol_t *self, int resend) {
    uint8_t rawpkt[100];
    ipcp_pkt_t *pkt = (ipcp_pkt_t *)rawpkt;
    int len = 0;
    uint8_t addr[4] = { 0, 0, 0, 0 }, dns[4] = { 0, 0, 0, 0 };

    (void)self;

    if(ipcp_state.ppp_state->netif) {
        addr[0] = ipcp_state.ppp_state->netif->ip_addr[0];
        addr[1] = ipcp_state.ppp_state->netif->ip_addr[1];
        addr[2] = ipcp_state.ppp_state->netif->ip_addr[2];
        addr[3] = ipcp_state.ppp_state->netif->ip_addr[3];

        dns[0] = ipcp_state.ppp_state->netif->dns[0];
        dns[1] = ipcp_state.ppp_state->netif->dns[1];
        dns[2] = ipcp_state.ppp_state->netif->dns[2];
        dns[3] = ipcp_state.ppp_state->netif->dns[3];
    }

    /* Fill in the code and identifier, then move onto the data. We'll get the
       length when we're done with the data. */
    pkt->code = LCP_CONFIGURE_REQUEST;

    if(resend)
        pkt->id = ipcp_state.last_conf;
    else
        pkt->id = ++ipcp_state.last_conf;

    pkt->data[len++] = IPCP_CONFIGURE_IP_ADDRESS;
    pkt->data[len++] = 6;
    pkt->data[len++] = addr[0];
    pkt->data[len++] = addr[1];
    pkt->data[len++] = addr[2];
    pkt->data[len++] = addr[3];

    pkt->data[len++] = IPCP_CONFIGURE_PRIMARY_DNS;
    pkt->data[len++] = 6;
    pkt->data[len++] = dns[0];
    pkt->data[len++] = dns[1];
    pkt->data[len++] = dns[2];
    pkt->data[len++] = dns[3];

    len += 4;
    pkt->len = htons(len);

    /* Set the resend timer for 3 seconds. */
    ipcp_state.next_resend = timer_ms_gettime64() + 3000;
    ipcp_state.resend_pkt = &ipcp_send_client_cfg;
    ipcp_state.resend_timeout = &ipcp_cfg_timeout;

    if(!resend)
        ipcp_state.resend_cnt = 10;

    return ppp_send(rawpkt, len, PPP_PROTOCOL_IPCP);
}

static int ipcp_send_code_reject(ppp_protocol_t *self, const uint8_t *pkt,
                                 size_t len) {
    uint8_t buf[len + 4];
    ipcp_pkt_t *out = (ipcp_pkt_t *)buf;
    uint16_t out_len = len + 4;

    (void)self;

    /* See if we need to truncate whatever is in the packet... */
    if(out_len > ipcp_state.ppp_state->peer_mru)
        out_len = ipcp_state.ppp_state->peer_mru;

    /* Fill in the packet itself. */
    out->code = LCP_CODE_REJECT;
    out->id = ++ipcp_state.last_coderej;
    out->len = htons(out_len);

    /* Copy in the data. */
    memcpy(buf + 4, pkt, out_len - 4);

    /* Send it away. */
    return ppp_send(buf, out_len, PPP_PROTOCOL_IPCP);
}

static int ipcp_send_terminate_ack(ppp_protocol_t *self, uint8_t id,
                                   const uint8_t *data, size_t len) {
    uint8_t buf[len + 4];
    ipcp_pkt_t *pkt = (ipcp_pkt_t *)buf;

    (void)self;

    /* Fill in the packet header. */
    pkt->code = LCP_TERMINATE_ACK;
    pkt->id = id;
    pkt->len = htons(len + 4);

    /* Copy the data over. */
    if(data)
        memcpy(buf + 4, data, len);

    /* Send it away. */
    return ppp_send(buf, len + 4, PPP_PROTOCOL_IPCP);
}

static int ipcp_handle_configure_req(ppp_protocol_t *self,
                                     const ipcp_pkt_t *pkt, size_t len) {
    size_t ptr = 0;
    uint8_t opt_len;
    uint8_t response[1500], nak[1500];
    uint8_t response_code = LCP_CONFIGURE_ACK;
    uint8_t response_len = 4, nak_len = 4;

    /* Parameters and their default values. */
    uint32_t addr = 0;

    (void)pkt;

    switch(ipcp_state.state) {
        case PPP_STATE_CLOSING:
        case PPP_STATE_STOPPING:
            /* Silently discard and don't move states. */
            return 0;

        case PPP_STATE_CLOSED:
            /* Send a terminate ack and discard the request. */
            return ipcp_send_terminate_ack(self, pkt->id, NULL, 0);

        case PPP_STATE_OPENED:
            /* XXXX: This layer down. */
            __fallthrough;

        case PPP_STATE_STOPPED:
            ipcp_send_client_cfg(self, 0);
            ipcp_state.state = PPP_STATE_REQUEST_SENT;
            break;
    }

    DBG("ipcp: Peer configure request received with opts:\n");

    /* Check each configuration option to see what we will reply with. */
    len -= 4;

    while(ptr < len) {
        if(len - ptr < 2) {
            /* Hopefully the peer will resend us a configure request with a sane
               set of options. For now, just ignore the bad request. */
            DBG("ipcp: bad configure length, ignoring.\n");
            return -1;
        }

        /* Parse out the length of this option. */
        opt_len = pkt->data[ptr + 1];

        if(opt_len < 2 || ptr + opt_len > len) {
            /* Another sign we have a bad packet. Just ignore it and wait for
               the peer to resend it. */
            DBG("ipcp: bad option length, ignoring packet\n");
            return -1;
        }

        /* Go through each parameter checking if it is sane. Check if each
           option is known to the implementation, whether it is of an
           appropriate length, and if the value is acceptable. */
        switch(pkt->data[ptr]) {
            case IPCP_CONFIGURE_IP_ADDRESS:
                if(opt_len == 6) {
                    addr = (pkt->data[ptr + 2] << 24) |
                        (pkt->data[ptr + 3] << 16) | (pkt->data[ptr + 4] << 8) |
                        pkt->data[ptr + 5];
                    DBG("    peer IP: %d.%d.%d.%d\n",
                        (int)pkt->data[ptr + 2], (int)pkt->data[ptr + 3],
                        (int)pkt->data[ptr + 4], (int)pkt->data[ptr + 5]);
                }
                else {
                    DBG("    peer IP (bad length)\n");
                    goto reject_opt;
                }
                break;

            case IPCP_CONFIGURE_PRIMARY_DNS:
                if(opt_len == 6) {
                    DBG("    primary DNS: %d.%d.%d.%d\n",
                        (int)pkt->data[ptr + 2], (int)pkt->data[ptr + 3],
                        (int)pkt->data[ptr + 4], (int)pkt->data[ptr + 5]);
                }
                else {
                    DBG("    primary DNS (bad length)\n");
                    goto reject_opt;
                }
                break;

            case IPCP_CONFIGURE_PRIMARY_NBNS:
                if(opt_len == 6) {
                    DBG("    primary NBNS: %d.%d.%d.%d\n",
                        (int)pkt->data[ptr + 2], (int)pkt->data[ptr + 3],
                        (int)pkt->data[ptr + 4], (int)pkt->data[ptr + 5]);
                }
                else {
                    DBG("    primary NBNS (bad length)\n");
                    goto reject_opt;
                }
                break;

            case IPCP_CONFIGURE_SECONDARY_DNS:
                if(opt_len == 6) {
                    DBG("    secondary DNS: %d.%d.%d.%d\n",
                        (int)pkt->data[ptr + 2], (int)pkt->data[ptr + 3],
                        (int)pkt->data[ptr + 4], (int)pkt->data[ptr + 5]);
                }
                else {
                    DBG("    secondary DNS (bad length)\n");
                    goto reject_opt;
                }
                break;

            case IPCP_CONFIGURE_SECONDARY_NBNS:
                if(opt_len == 6) {
                    DBG("    secondary NBNS: %d.%d.%d.%d\n",
                        (int)pkt->data[ptr + 2], (int)pkt->data[ptr + 3],
                        (int)pkt->data[ptr + 4], (int)pkt->data[ptr + 5]);
                }
                else {
                    DBG("    secondary NBNS (bad length)\n");
                    goto reject_opt;
                }
                break;

            /* If we don't know about the option, reject it. */
            default:
                DBG("    unknown option: %d (len %d)\n", pkt->data[ptr],
                    opt_len);
reject_opt:
                if(response_len + opt_len < 1500) {
                    response_code = LCP_CONFIGURE_REJECT;
                    memcpy(response + response_len, &pkt->data[ptr], opt_len);
                    response_len += opt_len;
                }
        }

        ptr += opt_len;
    }

    /* Respond to the packet appropriately. */
    if(response_code == LCP_CONFIGURE_ACK) {
        int rv = 0;
        ppp_state_t *st = ipcp_state.ppp_state;
        netif_t *nif = st->netif;

        memcpy(response, pkt, len + 4);
        response[0] = LCP_CONFIGURE_ACK;
        rv = ppp_send(response, len + 4, PPP_PROTOCOL_IPCP);

        /* Save all the accepted configuration options in the network
           device structure. */
        if(nif) {
            nif->gateway[0] = (uint8_t)(addr >> 24);
            nif->gateway[1] = (uint8_t)(addr >> 16);
            nif->gateway[2] = (uint8_t)(addr >> 8);
            nif->gateway[3] = (uint8_t)addr;
        }

        if(ipcp_state.state == PPP_STATE_ACK_RECEIVED) {
            ipcp_state.state = PPP_STATE_OPENED;

            /* The resend timer isn't running in the opened state, so clear it
               now. */
            ipcp_state.resend_pkt = NULL;
            ipcp_state.resend_timeout = NULL;

            /* XXXX: This layer up. */
            _ppp_enter_phase(PPP_PHASE_NETWORK);
        }
        else {
            ipcp_state.state = PPP_STATE_ACK_SENT;
        }

        return rv;
    }
    else if(response_code == LCP_CONFIGURE_REJECT) {
        ipcp_pkt_t *out = (ipcp_pkt_t *)response;

        out->code = LCP_CONFIGURE_REJECT;
        out->id = pkt->id;
        out->len = htons(response_len);

        if(ipcp_state.state != PPP_STATE_ACK_RECEIVED)
            ipcp_state.state = PPP_STATE_REQUEST_SENT;

        return ppp_send(response, response_len, PPP_PROTOCOL_IPCP);
    }
    else {
        ipcp_pkt_t *out = (ipcp_pkt_t *)nak;

        out->code = LCP_CONFIGURE_NAK;
        out->id = pkt->id;
        out->len = htons(nak_len);

        if(ipcp_state.state != PPP_STATE_ACK_RECEIVED)
            ipcp_state.state = PPP_STATE_REQUEST_SENT;

        return ppp_send(nak, nak_len, PPP_PROTOCOL_IPCP);
    }

    return 0;
}

static int ipcp_handle_configure_ack(ppp_protocol_t *self,
                                     const ipcp_pkt_t *pkt, size_t len) {
    (void)self;
    (void)len;

    if(pkt->id != ipcp_state.last_conf) {
        DBG("ipcp: received configure ack with an invalid identifier\n");
        return -1;
    }

    DBG("ipcp: received configure ack\n");

    /* XXXX: It would be a good idea to make sure the peer ACKed everything we
       sent properly... */

    switch(ipcp_state.state) {
        case PPP_STATE_CLOSING:
        case PPP_STATE_STOPPING:
            /* Silently discard. */
            return 0;

        case PPP_STATE_CLOSED:
        case PPP_STATE_STOPPED:
            return ipcp_send_terminate_ack(self, pkt->id, NULL, 0);

        case PPP_STATE_REQUEST_SENT:
            /* The state diagram says to initialize the resend counter here, but
               that doesn't really make much sense to me... Oh well, I guess the
               spec is probably right? */
            ipcp_state.resend_cnt = 10;
            ipcp_state.state = PPP_STATE_ACK_RECEIVED;
            return 0;

        case PPP_STATE_OPENED:
            /* Uh oh... Something's gone wrong. */
            /* XXXX: This layer down. */
            __fallthrough;

        case PPP_STATE_ACK_RECEIVED:
            /* Well, this isn't good... Resend the configure request and back
               down to the request sent state we go... */
            ipcp_state.state = PPP_STATE_REQUEST_SENT;
            return ipcp_send_client_cfg(self, 0);

        case PPP_STATE_ACK_SENT:
            ipcp_state.resend_pkt = NULL;
            ipcp_state.resend_timeout = NULL;
            ipcp_state.state = PPP_STATE_OPENED;
            /* XXXX: This layer up. */
            _ppp_enter_phase(PPP_PHASE_NETWORK);

            return 0;
    }

    return 0;
}

static int ipcp_handle_configure_nak(ppp_protocol_t *self,
                                     const ipcp_pkt_t *pkt, size_t len) {
    uint32_t addr = 0, dns = 0;
    size_t ptr = 0;
    uint8_t opt_len;

    (void)self;

    if(ipcp_state.ppp_state->netif)
        addr = net_ipv4_address(ipcp_state.ppp_state->netif->ip_addr);

    if(pkt->id != ipcp_state.last_conf) {
        DBG("ipcp: received configure nak with an invalid identifier\n");
        return -1;
    }

    switch(ipcp_state.state) {
        case PPP_STATE_CLOSING:
        case PPP_STATE_STOPPING:
            /* Silently discard and don't move states. */
            return 0;

        case PPP_STATE_CLOSED:
        case PPP_STATE_STOPPED:
            /* Send a terminate ack and discard the request. */
            return ipcp_send_terminate_ack(self, pkt->id, NULL, 0);

        case PPP_STATE_OPENED:
            /* XXXX: This layer down. */
            __fallthrough;

        case PPP_STATE_REQUEST_SENT:
        case PPP_STATE_ACK_RECEIVED:
            ipcp_state.state = PPP_STATE_REQUEST_SENT;
            break;
    }

    DBG("ipcp: peer sent configure nak with opts:\n");

    /* Check each configuration option to see what we will reply with. */
    len -= 4;

    while(ptr < len) {
        if(len - ptr < 2) {
            /* Hopefully the peer will resend us a configure nak with a sane
               set of options. For now, just ignore the bad request. */
            DBG("ipcp: bad configure length, ignoring.\n");
            return -1;
        }

        /* Parse out the length of this option. */
        opt_len = pkt->data[ptr + 1];

        if(opt_len < 2 || ptr + opt_len > len) {
            /* Another sign we have a bad packet. Just ignore it and wait for
               the peer to resend it. */
            DBG("ipcp: bad option length, ignoring packet\n");
            return -1;
        }

        /* Go through each parameter checking if it is sane. Check if each
           option is known to the implementation, whether it is of an
           appropriate length, and if the value is acceptable.
           We ignore any options with lengths we don't understand. */
        switch(pkt->data[ptr]) {
            case IPCP_CONFIGURE_IP_ADDRESS:
                if(opt_len == 6) {
                    addr = (pkt->data[ptr + 2] << 24) |
                        (pkt->data[ptr + 3] << 16) | (pkt->data[ptr + 4] << 8) |
                        pkt->data[ptr + 5];
                    DBG("    our IP: %d.%d.%d.%d\n", (int)pkt->data[ptr + 2],
                        (int)pkt->data[ptr + 3], (int)pkt->data[ptr + 4],
                        (int)pkt->data[ptr + 5]);
                }
                else {
                    DBG("    our IP (bad length)\n");
                }
                break;

            case IPCP_CONFIGURE_PRIMARY_DNS:
                if(opt_len == 6) {
                    dns = (pkt->data[ptr + 2] << 24) |
                        (pkt->data[ptr + 3] << 16) | (pkt->data[ptr + 4] << 8) |
                        pkt->data[ptr + 5];
                    DBG("    DNS 1: %d.%d.%d.%d\n", (int)pkt->data[ptr + 2],
                        (int)pkt->data[ptr + 3], (int)pkt->data[ptr + 4],
                        (int)pkt->data[ptr + 5]);
                }
                else {
                    DBG("    DNS 1 (bad length)\n");
                }
                break;

            /* If we don't know about the option, ignore it. */
            default:
                DBG("    unknown option: %d (len %d)\n", pkt->data[ptr],
                    opt_len);
        }

        ptr += opt_len;
    }

    /* Save the options that the other side requested and resend the configure
       request. */
    if(ipcp_state.ppp_state->netif) {
        netif_t *nif = ipcp_state.ppp_state->netif;

        nif->ip_addr[0] = (uint8_t)(addr >> 24);
        nif->ip_addr[1] = (uint8_t)(addr >> 16);
        nif->ip_addr[2] = (uint8_t)(addr >> 8);
        nif->ip_addr[3] = (uint8_t)addr;

        nif->dns[0] = (uint8_t)(dns >> 24);
        nif->dns[1] = (uint8_t)(dns >> 16);
        nif->dns[2] = (uint8_t)(dns >> 8);
        nif->dns[3] = (uint8_t)dns;
    }

    return ipcp_send_client_cfg(self, 0);
}

static int ipcp_handle_terminate_req(ppp_protocol_t *self,
                                     const ipcp_pkt_t *pkt, size_t len) {
    (void)len;

    switch(ipcp_state.state) {
        case PPP_STATE_STOPPED:
        case PPP_STATE_CLOSED:
        case PPP_STATE_CLOSING:
        case PPP_STATE_STOPPING:
        case PPP_STATE_REQUEST_SENT:
            /* No state transitions needed. */
            break;

        case PPP_STATE_ACK_RECEIVED:
        case PPP_STATE_ACK_SENT:
            ipcp_state.state = PPP_STATE_REQUEST_SENT;
            break;

        case PPP_STATE_OPENED:
            /* XXXX: This layer down. */
            ipcp_state.resend_pkt = NULL;
            ipcp_state.resend_timeout = NULL;
            ipcp_state.state = PPP_STATE_STOPPING;
            __fallthrough;

        default:
            return -1;
    }

    return ipcp_send_terminate_ack(self, pkt->id, NULL, 0);
}

static int ipcp_handle_terminate_ack(ppp_protocol_t *self,
                                     const ipcp_pkt_t *pkt, size_t len) {
    (void)len;
    (void)self;

    if(pkt->id != ipcp_state.last_term) {
        DBG("ipcp: received terminate ack with an invalid identifier\n");
        return -1;
    }

    switch(ipcp_state.state) {
        case PPP_STATE_STOPPED:
        case PPP_STATE_CLOSED:
        case PPP_STATE_REQUEST_SENT:
        case PPP_STATE_ACK_SENT:
            /* Nothing to do. */
            break;

        case PPP_STATE_CLOSING:
            /* XXXX: This layer finished. */
            ipcp_state.resend_pkt = NULL;
            ipcp_state.resend_timeout = NULL;
            ipcp_state.state = PPP_STATE_CLOSED;
            break;

        case PPP_STATE_STOPPING:
            /* XXXX: This layer finished. */
            ipcp_state.resend_pkt = NULL;
            ipcp_state.resend_timeout = NULL;
            ipcp_state.state = PPP_STATE_STOPPED;
            break;

        case PPP_STATE_ACK_RECEIVED:
            ipcp_state.state = PPP_STATE_REQUEST_SENT;
            break;

        case PPP_STATE_OPENED:
            /* XXXX: This layer down. */
            /* Something has gone wrong... Attempt to reconfigure the link. */
            ipcp_state.state = PPP_STATE_REQUEST_SENT;
            return ipcp_send_client_cfg(self, 0);

        default:
            return -1;
    }

    return 0;
}

/* PPP Protocol implementation functions. */
static int ipcp_shutdown(ppp_protocol_t *self) {
    return ppp_del_protocol(self);
}

static int ipcp_input(ppp_protocol_t *self, const uint8_t *buf, size_t len) {
    const ipcp_pkt_t *pkt = (const ipcp_pkt_t *)buf;

    (void)self;

    /* Sanity check. */
    if(len < sizeof(ipcp_pkt_t) || len != ntohs(pkt->len)) {
        /* Drop bad packet. Should probably log this at some point. */
        return -1;
    }

    /* What type of packet did we get? */
    switch(pkt->code) {
        case LCP_CONFIGURE_REQUEST:
            return ipcp_handle_configure_req(self, pkt, len);

        case LCP_CONFIGURE_ACK:
            return ipcp_handle_configure_ack(self, pkt, len);

        case LCP_CONFIGURE_NAK:
            return ipcp_handle_configure_nak(self, pkt, len);

        case LCP_CONFIGURE_REJECT:
            //return ipcp_handle_configure_rej(self, pkt, len);
            return 0;

        case LCP_TERMINATE_REQUEST:
            return ipcp_handle_terminate_req(self, pkt, len);

        case LCP_TERMINATE_ACK:
            return ipcp_handle_terminate_ack(self, pkt, len);

        case LCP_CODE_REJECT:
            /* This is only important if we're in the ack received state... We
               probably should log it though at some point... */
            if(ipcp_state.state == PPP_STATE_ACK_RECEIVED)
                ipcp_state.state = PPP_STATE_REQUEST_SENT;
            break;

        default:
            /* We don't know what this is, so send a code reject. */
            return ipcp_send_code_reject(self, buf, len);
    }

    return 0;
}

static void ipcp_enter_phase(ppp_protocol_t *self, int oldp, int newp) {
    (void)self;
    (void)oldp;

    /* We only care about when we're entering the network phase. */
    if(newp == PPP_PHASE_NETWORK) {
        ipcp_send_client_cfg(self, 0);
        ipcp_state.state = PPP_STATE_REQUEST_SENT;
    }
}

static void ipcp_check_timeouts(ppp_protocol_t *self, uint64_t tm) {
    (void)self;
    (void)tm;

    if(ipcp_state.resend_pkt && tm >= ipcp_state.next_resend) {
        if(!ipcp_state.resend_cnt) {
            ipcp_state.resend_timeout(self);
        }
        else {
            ipcp_state.resend_pkt(self, 1);
            --ipcp_state.resend_cnt;
        }
    }
}

static int ip_input(ppp_protocol_t *self, const uint8_t *buf, size_t len) {
    (void)self;

    if(ipcp_state.state == PPP_STATE_OPENED)
        return net_ipv4_input(ipcp_state.ppp_state->netif, buf, len, NULL);

    /* If we're not open, silently discard the packet. */
    return 0;
}

static ppp_protocol_t ipcp_proto = {
    PPP_PROTO_ENTRY_INIT,
    "ipcp",
    PPP_PROTOCOL_IPCP,
    NULL,                   /* privdata */
    NULL,                   /* init */
    &ipcp_shutdown,
    &ipcp_input,
    &ipcp_enter_phase,
    &ipcp_check_timeouts
};

static ppp_protocol_t ip_proto = {
    PPP_PROTO_ENTRY_INIT,
    "ipv4",
    PPP_PROTOCOL_IPv4,
    NULL,                   /* privdata */
    NULL,                   /* init */
    &ipcp_shutdown,
    &ip_input,
    NULL,                   /* enter_phase */
    NULL                    /* check_timeouts */
};

int _ppp_ipcp_init(ppp_state_t *st) {
    (void)st;

    ipcp_state.ppp_state = st;

    return ppp_add_protocol(&ip_proto) | ppp_add_protocol(&ipcp_proto);
}
