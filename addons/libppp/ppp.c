/* KallistiOS ##version##

   libppp/ppp.c
   Copyright (C) 2007, 2014 Lawrence Sebald
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include <kos/mutex.h>
#include <kos/sem.h>

#include <arch/timer.h>

#include <ppp/ppp.h>

#include "ppp_internal.h"
#include "fcs.h"

#define FLAG_SEQUENCE 0x7e
#define ADDRESS_FIELD 0xff
#define CONTROL_FIELD 0x03

#define ESCAPE_CHAR   0x7d

/* What do we expect next? */
#define EXPECT_FLAGSEQ 1
#define EXPECT_ADDRESS 2
#define EXPECT_CONTROL 3
#define EXPECT_DATA    4

static ppp_state_t ppp_state;
static mutex_t mutex = RECURSIVE_MUTEX_INITIALIZER;
static semaphore_t established_sem = SEM_INITIALIZER(0);
static kthread_t *waiting_thd = NULL;
static int conn_rv = 0;

/* Receive buffer. This should never be touched by anything but the PPP thread.
   1504 bytes = 1500 byte MRU + 2 bytes for protocol + 2 bytes for FCS. */
#define PPP_MRU 1500
static uint8_t ppp_recvbuf[PPP_MRU + 4];
static size_t ppp_recvbuf_len;

TAILQ_HEAD(ppp_proto_list, ppp_proto);
static struct ppp_proto_list protocols = TAILQ_HEAD_INITIALIZER(protocols);

/* Forward declaration. */
static netif_t ppp_if;

#ifdef PPP_DEBUG
const char *phases[5] = {
    "Dead", "Establish", "Authenticate", "Network", "Terminate"
};
#endif

static inline void set_accm_bit(uint32_t *accm, uint8_t bit) {
    int pos1 = bit >> 5;
    int pos2 = bit & 0x1F;

    accm[pos1] |= (1 << pos2);
}

static inline int check_accm_bit(uint32_t *accm, uint8_t bit) {
    int pos1 = bit >> 5;
    int pos2 = bit & 0x1F;

    return accm[pos1] & (1 << pos2);
}

int ppp_send(const uint8_t *data, size_t len, uint16_t proto) {
    uint8_t tmp[5];
    uint16_t fcs = INITIAL_FCS;
    size_t i, j, run_len = 0;
    const uint8_t *run_start = data;

    /* We can't use mutex_lock() inside an IRQ, so we have this song and dance
       with mutex_trylock() instead in that case. */
    if(irq_inside_int()) {
        if(mutex_trylock(&mutex)) {
            errno = EAGAIN;
            return -1;
        }
    }
    else {
        mutex_lock(&mutex);
    }

    if(!ppp_state.device) {
        mutex_unlock(&mutex);
        errno = ENETDOWN;
        return -1;
    }

    if(ppp_state.phase == PPP_PHASE_DEAD) {
        mutex_unlock(&mutex);
        errno = ENETDOWN;
        return -1;
    }

    /* Send the start of the framing out first. */
    i = 0;
    tmp[i++] = FLAG_SEQUENCE;

    if(check_accm_bit(ppp_state.out_accm, ADDRESS_FIELD)) {
        tmp[i++] = ESCAPE_CHAR;
        tmp[i++] = ADDRESS_FIELD ^ 0x20;
    }
    else {
        tmp[i++] = ADDRESS_FIELD;
    }

    if(check_accm_bit(ppp_state.out_accm, CONTROL_FIELD)) {
        tmp[i++] = ESCAPE_CHAR;
        tmp[i++] = CONTROL_FIELD ^ 0x20;
    }
    else {
        tmp[i++] = CONTROL_FIELD;
    }

    fcs = (fcs >> 8) ^ fcstab[(fcs ^ ADDRESS_FIELD) & 0xFF];
    fcs = (fcs >> 8) ^ fcstab[(fcs ^ CONTROL_FIELD) & 0xFF];

    ppp_state.device->tx(ppp_state.device, tmp, i, 0);

    /* Next deal with the PPP protocol field. */
    i = 0;

    if(check_accm_bit(ppp_state.out_accm, (uint8_t)(proto >> 8))) {
        tmp[i++] = ESCAPE_CHAR;
        tmp[i++] = ((uint8_t)(proto >> 8)) ^ 0x20;
    }
    else {
        tmp[i++] = (uint8_t)(proto >> 8);
    }

    if(check_accm_bit(ppp_state.out_accm, (uint8_t)proto)) {
        tmp[i++] = ESCAPE_CHAR;
        tmp[i++] = ((uint8_t)proto) ^ 0x20;
    }
    else {
        tmp[i++] = (uint8_t)proto;
    }

    fcs = (fcs >> 8) ^ fcstab[(fcs ^ ((uint8_t)(proto >> 8))) & 0xFF];
    fcs = (fcs >> 8) ^ fcstab[(fcs ^ ((uint8_t)proto)) & 0xFF];

    ppp_state.device->tx(ppp_state.device, tmp, i, 0);

    /* Deal with the data now. */
    for(j = 0; j < len; ++j) {
        if(check_accm_bit(ppp_state.out_accm, data[j])) {
            /* Send everything up to this point. */
            if(run_len)
                ppp_state.device->tx(ppp_state.device, run_start, run_len, 0);

            /* Escape the current byte and send it. */
            tmp[0] = ESCAPE_CHAR;
            tmp[1] = data[j] ^ 0x20;
            ppp_state.device->tx(ppp_state.device, tmp, 2, 0);

            /* Restart the run counter. */
            run_start = data + j + 1;
            run_len = 0;
        }
        else {
            ++run_len;
        }

        fcs = (fcs >> 8) ^ fcstab[(fcs ^ data[j]) & 0xFF];
    }

    /* Send anything left over. */
    if(run_len)
        ppp_state.device->tx(ppp_state.device, run_start, run_len, 0);

    /* Finish up with the FCS and tack it onto the end along with an extra flag
       sequence to mark the end of the packet. */
    fcs = fcs ^ 0xFFFF;
    i = 0;

    if(check_accm_bit(ppp_state.out_accm, (uint8_t)(fcs & 0xFF))) {
        tmp[i++] = ESCAPE_CHAR;
        tmp[i++] = ((uint8_t)(fcs & 0xFF)) ^ 0x20;
    }
    else {
        tmp[i++] = (uint8_t)(fcs & 0xFF);
    }

    if(check_accm_bit(ppp_state.out_accm, (uint8_t)((fcs >> 8) & 0xFF))) {
        tmp[i++] = ESCAPE_CHAR;
        tmp[i++] = ((uint8_t)((fcs >> 8) & 0xFF)) ^ 0x20;
    }
    else {
        tmp[i++] = (uint8_t)((fcs >> 8) & 0xFF);
    }

    tmp[i++] = FLAG_SEQUENCE;

    ppp_state.device->tx(ppp_state.device, tmp, i, PPP_TX_END_OF_PKT);

    /* Clean up, we're done. */
    mutex_unlock(&mutex);

    return 0;
}

static int ppp_input(void) {
    uint16_t proto;
    uint8_t *ptr;
    size_t len;
    ppp_protocol_t *i;

    /* Do we have a compressed protocol value? */
    if(ppp_recvbuf[0] & 0x01) {
        proto = ppp_recvbuf[0];
        ptr = ppp_recvbuf + 1;
        len = ppp_recvbuf_len - 3;
    }
    else {
        proto = (ppp_recvbuf[0] << 8) | ppp_recvbuf[1];
        ptr = ppp_recvbuf + 2;
        len = ppp_recvbuf_len - 4;
    }

    /* Look for the specified protocol in the list of registered protocols. */
    TAILQ_FOREACH(i, &protocols, entry) {
        if(i->code == proto)
            return i->input(i, ptr, len);
    }

    /* We didn't find it in the protocols list, so send a protocol reject. */
    return ppp_lcp_send_proto_reject(proto, ppp_recvbuf + 2,
                                     ppp_recvbuf_len - 4);
}

/* PPP thread function. */
void *ppp_main(void *arg) {
    const uint8_t *data, *cur;
    uint8_t ch;
    ssize_t data_len;
    int esc = 0, expect = EXPECT_FLAGSEQ;
    uint16_t fcs = INITIAL_FCS;
    ppp_protocol_t *i;
    uint64_t now;

    (void)arg;

    /* Begin establishing a connection. */
    _ppp_enter_phase(PPP_PHASE_ESTABLISH);

    /* Loop as long as the link is open. */
    while(ppp_state.phase != PPP_PHASE_DEAD) {
        mutex_lock(&mutex);

        /* Check if there's any data waiting for us on the device. */
        if(!(cur = data = ppp_state.device->rx(ppp_state.device, &data_len))) {
            goto check_timeouts;
        }

        while(data_len--) {
            ch = *cur++;

            /* Go through some special edge cases... */
            if(ch == FLAG_SEQUENCE) {
                /* A flag sequence marks the beginning of a packet. Check what
                   we should do with whatever we have already. */
                switch(expect) {
                    case EXPECT_FLAGSEQ:
                        expect = EXPECT_ADDRESS;
                        break;

                    case EXPECT_ADDRESS:
                        /* Empty packet (or time fill). No matter, just continue
                           expecting the all points address next. */
                        break;

                    case EXPECT_CONTROL:
                        DBG("ppp: aborting packet, unexpected flag sequence\n");

                        expect = EXPECT_ADDRESS;
                        ppp_recvbuf_len = 0;
                        fcs = INITIAL_FCS;
                        break;

                    case EXPECT_DATA:
                        /* Check the FCS so far, and see if the packet is valid.
                           If it is, then pass the packet along to the proper
                           protocol handler. */
                        if(fcs == FINAL_FCS) {
                            /* This is a good packet, pass it along. */
                            ppp_input();
                        }
                        else    {
                            DBG("ppp: dropping packet with bad final fcs, got: "
                                "%04x\n", fcs);
                            DBG("ppp: was for proto %02x%02x\n", ppp_recvbuf[0],
                                ppp_recvbuf[1]);
                            DBG("ppp: was %d bytes long\n",
                                (int)ppp_recvbuf_len);
                        }

                        expect = EXPECT_ADDRESS;
                        fcs = INITIAL_FCS;
                        ppp_recvbuf_len = 0;
                        break;
                }
            }
            else if(ch == ESCAPE_CHAR) {
                esc = 1;
            }
            else if(check_accm_bit(ppp_state.in_accm, ch)) {
                DBG("ppp: dropping character that should be escaped: %02x\n",
                    ch);
            }
            else {
                if(esc) {
                    ch ^= 0x20;
                    esc = 0;
                }

                /* Update our FCS */
                fcs = (fcs >> 8) ^ fcstab[(fcs ^ ch) & 0xFF];

                switch(expect) {
                    case EXPECT_FLAGSEQ:
                        DBG("ppp: Got data byte while expecting flag sequence, "
                            "dropping %02x\n", ch);

                        fcs = INITIAL_FCS;
                        break;

                    case EXPECT_CONTROL:
                        if(ch == CONTROL_FIELD) {
                            /* We got the correct control field, move onto
                               data. */
                            expect = EXPECT_DATA;
                        }
                        else {
                            /* Something is probably wrong, so go ahead and drop
                               the packet now. */
                            expect = EXPECT_FLAGSEQ;
                            ppp_recvbuf_len = 0;
                            fcs = INITIAL_FCS;

                            DBG("ppp: Dropping packet with unexpected control "
                                "field: %02x\n", ch);
                        }
                        break;

                    case EXPECT_ADDRESS:
                        if(ch == ADDRESS_FIELD) {
                            /* We got the address byte, next we should get the
                               control byte. */
                            expect = EXPECT_CONTROL;
                            break;
                        }
                        else {
                            /* Otherwise, the address/control fields were
                               hopefully compressed. Check the flags set by LCP
                               to make sure. */
                            expect = EXPECT_DATA;
                            __fallthrough;
                        }

                    case EXPECT_DATA:
                        if(ppp_recvbuf_len < PPP_MRU + 4) {
                            ppp_recvbuf[ppp_recvbuf_len++] = ch;
                        }
                        else {
                            /* We've gone beyond the MRU, so bail. */
                            expect = EXPECT_FLAGSEQ;
                            ppp_recvbuf_len = 0;
                            fcs = INITIAL_FCS;

                            DBG("ppp: Dropping packet with length greater than "
                                "the configured MRU\n");
                        }
                        break;
                }
            }
        }

check_timeouts:
        /* Check if there are any timeouts on any of the protocols. */
        now = timer_ms_gettime64();

        TAILQ_FOREACH(i, &protocols, entry) {
            if(i->check_timeouts)
                i->check_timeouts(i, now);
        }

        mutex_unlock(&mutex);
        thd_pass();
    }

    return NULL;
}

/* Register a PPP protocol handler */
int ppp_add_protocol(ppp_protocol_t *hnd) {
    if(hnd->init) {
        if(hnd->init(hnd))
            return -1;
    }

    /* XXXX: Probably should look for duplicates. Oh well. */
    TAILQ_INSERT_TAIL(&protocols, hnd, entry);

    return 0;
}

int ppp_del_protocol(ppp_protocol_t *hnd) {
    /* This is usually called from the protocol shutdown, so assume that the
       protocol is already shutting down and just remove it from the list. */
    TAILQ_REMOVE(&protocols, hnd, entry);

    return 0;
}

int ppp_set_device(ppp_device_t *dev) {
    mutex_lock(&mutex);

    if(!ppp_state.initted) {
        mutex_unlock(&mutex);
        return -1;
    }

    if(ppp_state.phase != PPP_PHASE_DEAD) {
        mutex_unlock(&mutex);
        return -1;
    }

    ppp_state.device = dev;
    mutex_unlock(&mutex);

    return 0;
}

int ppp_set_login(const char *username, const char *password) {
    char *un = NULL, *pw = NULL;

    mutex_lock(&mutex);

    if(!ppp_state.initted) {
        mutex_unlock(&mutex);
        return -1;
    }

    if(username) {
        if(!(un = (char *)malloc(strlen(username) + 1))) {
            mutex_unlock(&mutex);
            return -1;
        }

        strcpy(un, username);
    }

    if(password) {
        if(!(pw = (char *)malloc(strlen(password) + 1))) {
            if(username)
                free(un);
            mutex_unlock(&mutex);
            return -1;
        }

        strcpy(pw, password);
    }

    /* Clean up any existing values. */
    free(ppp_state.username);
    free(ppp_state.passwd);

    /* Save the new values into the ppp state. */
    ppp_state.username = un;
    ppp_state.passwd = pw;

    mutex_unlock(&mutex);

    return 0;
}

uint32_t ppp_get_flags(void) {
    return ppp_state.our_flags;
}

uint32_t ppp_get_peer_flags(void) {
    return ppp_state.peer_flags;
}

void ppp_set_flags(uint32_t flags) {
    ppp_state.our_flags = flags;
}

int _ppp_enter_phase(int phase) {
    int old;
    ppp_protocol_t *i;

    if(phase < PPP_PHASE_DEAD || phase > PPP_PHASE_TERMINATE)
        return -1;

    DBG("ppp: entering phase %s\n", phases[phase - 1]);

    mutex_lock(&mutex);

    if(!ppp_state.initted) {
        mutex_unlock(&mutex);
        return -1;
    }

    /* XXXX: We should probably check that this is a valid phase transition, but
       that's not all that important... Probably. */
    old = ppp_state.phase;
    ppp_state.phase = phase;

    /* Let each protocol know, if they care to monitor phase transitions. */
    if(phase != old) {
        TAILQ_FOREACH(i, &protocols, entry) {
            if(i->enter_phase)
                i->enter_phase(i, old, phase);
        }
    }

    /* See if we need to wake up a thread that may be waiting for link
       establishment... */
    if(waiting_thd && ((phase == old && phase == PPP_PHASE_NETWORK) ||
                       phase == PPP_PHASE_TERMINATE ||
                       phase == PPP_PHASE_DEAD)) {
        conn_rv = phase == PPP_PHASE_NETWORK ? 0 : -1;
        sem_signal(&established_sem);
        waiting_thd = NULL;
    }

    mutex_unlock(&mutex);
    return 0;
}

int ppp_connect(void) {
    mutex_lock(&mutex);

    /* Make sure the library is initted to start with. */
    if(!ppp_state.initted) {
        mutex_unlock(&mutex);
        return -1;
    }

    /* Make sure we aren't already connected. */
    if(ppp_state.phase != PPP_PHASE_DEAD) {
        mutex_unlock(&mutex);
        return -1;
    }

    /* Make sure we have a functioning device up and running. */
    if(!ppp_state.device || ppp_state.device->detect(ppp_state.device) < 0 ||
       ppp_state.device->init(ppp_state.device) < 0) {
        mutex_unlock(&mutex);
        return -1;
    }

    /* Start the PPP thread, which will establish the connection for us. */
    if(!(ppp_state.thd = thd_create(0, &ppp_main, NULL))) {
        ppp_state.device->shutdown(ppp_state.device);
        mutex_unlock(&mutex);
        return -1;
    }

    net_set_default(&ppp_if);

    /* Wait for the link to be established. */
    waiting_thd = thd_get_current();
    conn_rv = 0;
    mutex_unlock(&mutex);
    sem_wait(&established_sem);

    return conn_rv;
}

static int ppp_if_dummy(netif_t *self) {
    (void)self;
    return 0;
}

static int ppp_if_shutdown(netif_t *self) {
    (void)self;
    return ppp_shutdown();
}

static int ppp_if_tx(netif_t *self, const uint8_t *data, int len, int blocking) {
    (void)self;
    (void)blocking;

    /* XXXX: Support protocols other than IPv4 here... */
    return ppp_send(data, len, PPP_PROTOCOL_IPv4);
}

static int ppp_if_set_flags(netif_t *self, uint32_t flags_and, uint32_t flags_or) {
    self->flags = (self->flags & flags_and) | flags_or;
    return 0;
}


static int ppp_if_set_mc(netif_t *self, const uint8_t *list, int count) {
    (void)self;
    (void)list;
    (void)count;

    /* No multicasting on ppp... */
    return 0;
}

/* KOS Network interface. */
static netif_t ppp_if = {
    { 0 },
    "ppp",
    "Point-to-Point Protocol",
    0,                          /* index */
    0,                          /* dev_id */
    NETIF_NOETH,                /* flags */
    { 0, 0, 0, 0, 0, 0 },       /* MAC address */
    { 0, 0, 0, 0 },             /* IPv4 address */
    { 255, 255, 255, 255 },     /* netmask */
    { 0, 0, 0, 0 },             /* gateway */
    { 0, 0, 0, 0 },             /* broadcast */
    { 0, 0, 0, 0 },             /* dns */
    1496,                       /* mtu */
    IN6ADDR_ANY_INIT,           /* ip6_lladdr */
    NULL,                       /* ip6_addrs */
    0,                          /* ip6_addr_count */
    IN6ADDR_ANY_INIT,           /* ip6_gateway */
    0,                          /* mtu6 */
    255,                        /* hop_limit */
    &ppp_if_dummy,              /* detect */
    &ppp_if_dummy,              /* init */
    &ppp_if_shutdown,           /* shutdown */
    &ppp_if_dummy,              /* start */
    &ppp_if_dummy,              /* stop */
    &ppp_if_tx,                 /* tx */
    &ppp_if_dummy,              /* tx_commit */
    &ppp_if_dummy,              /* rx_poll */
    &ppp_if_set_flags,          /* set_flags */
    &ppp_if_set_mc              /* set_mc */
};

int ppp_init(void) {
    mutex_lock(&mutex);

    if(ppp_state.initted) {
        mutex_unlock(&mutex);
        return -1;
    }

    memset(&ppp_state, 0, sizeof(ppp_state));

    ppp_state.initted = 1;
    ppp_state.state = PPP_STATE_INITIAL;
    ppp_state.phase = PPP_PHASE_DEAD;

    /* Make sure that the escape and flag characters are always escaped. */
    set_accm_bit(ppp_state.out_accm, ESCAPE_CHAR);
    set_accm_bit(ppp_state.out_accm, FLAG_SEQUENCE);
    set_accm_bit(ppp_state.in_accm, ESCAPE_CHAR);
    set_accm_bit(ppp_state.in_accm, FLAG_SEQUENCE);
    ppp_state.out_accm[0] = 0xffffffff;
    ppp_state.peer_mru = 1500;
    ppp_state.netif = &ppp_if;

    /* Initialize a few sane defaults for the LCP configuration. */
    ppp_state.our_magic = time(NULL);
    ppp_state.our_flags = PPP_FLAG_ACCOMP |
        PPP_FLAG_MAGIC_NUMBER;

    /* Initialize all the protocols that are included in the library. */
    _ppp_lcp_init(&ppp_state);
    _ppp_pap_init(&ppp_state);
    _ppp_ipcp_init(&ppp_state);

    /* Add us to netcore. */
    net_reg_device(&ppp_if);

    mutex_unlock(&mutex);
    return 0;
}

int ppp_shutdown(void) {
    ppp_protocol_t *i, *next;

    /* If we're in an int, it's ok if we can't lock */
    if(irq_inside_int())
        mutex_trylock(&mutex);
    else
        mutex_lock(&mutex);

    if(!ppp_state.initted) {
        mutex_unlock(&mutex);
        return -1;
    }

    /* Shut down any protocols that are still up. */
    i = TAILQ_FIRST(&protocols);
    while(i) {
        next = TAILQ_NEXT(i, entry);

        if(i->shutdown)
            i->shutdown(i);
        else
            ppp_del_protocol(i);

        i = next;
    }

    /* XXXX: Do real cleanup... */
    ppp_state.initted = 0;

    net_unreg_device(&ppp_if);

    mutex_unlock(&mutex);
    return 0;
}
