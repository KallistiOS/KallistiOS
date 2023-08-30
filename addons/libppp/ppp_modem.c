/* KallistiOS ##version##

   libppp/ppp_modem.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <stdio.h>
#include <stdint.h>

#include <kos/dbglog.h>
#include <kos/thread.h>

#include <arch/timer.h>

#include <dc/modem/modem.h>

#include <ppp/ppp.h>

static int ppp_modem_detect_init(ppp_device_t *self) {
    (void)self;

    return 0;
}

static int ppp_modem_shutdown(ppp_device_t *self) {
    (void)self;

    if(modem_is_connected())
        modem_disconnect();

    modem_shutdown();
    return 0;
}

static int ppp_modem_tx(ppp_device_t *self, const uint8_t *data, size_t len,
                        uint32_t flags) {
    size_t done = 0;
    (void)self;
    (void)flags;

    /* As long as we have something to send, do so. */
    while(done < len) {
        /* FIXME: This will loop infinitely if modem_write_data always fails */
        done += modem_write_data((unsigned char *)data + done, (int)len - done);
    }

    return 0;
}

static const uint8_t *ppp_modem_rx(ppp_device_t *self, ssize_t *out_len) {
    static uint8_t rb[1024];
    int cnt = 0;

    (void)self;

    /* Read anything that's waiting. */
    cnt = modem_read_data((unsigned char *)rb, 1024);

    /* If we got anything, return what we got. */
    if(cnt) {
        *out_len = (ssize_t)cnt;
        return rb;
    }

    *out_len = 0;
    return NULL;
}

static ppp_device_t modem_dev = {
    "modem",                            /* name */
    "PPP over Dreamcast Modem",         /* descr */
    0,                                  /* index */
    0,                                  /* flags */
    NULL,                               /* privdata */
    &ppp_modem_detect_init,             /* detect */
    &ppp_modem_detect_init,             /* init */
    &ppp_modem_shutdown,                /* shutdown */
    &ppp_modem_tx,                      /* tx */
    &ppp_modem_rx                       /* rx */
};

int ppp_modem_init(const char *number, int blind, int *conn_rate) {
    uint64_t timeout;

    /* Initialize the modem. */
    if(!modem_init())
        return -1;

    /* Set the modem up to connect to a remote machine. */
    modem_set_mode(MODEM_MODE_REMOTE, MODEM_SPEED_V8_AUTO);

    /* If we aren't doing blind dialing, wait up to 5 seconds for a dialtone. */
    if(!blind) {
        if(modem_wait_dialtone(5000)) {
            modem_shutdown();
            return -2;
        }
    }

    /* Dial the specified phone number. */
    if(!modem_dial(number)) {
        modem_shutdown();
        return -3;
    }

    /* Give ourselves a 60 second timeout to establish a connection. */
    timeout = timer_ms_gettime64() + 60 * 1000;
    while(timer_ms_gettime64() < timeout && modem_is_connecting()) {
        thd_pass();
    }

    /* Did we connect successfully? */
    if(!modem_is_connected()) {
        modem_shutdown();
        return -4;
    }

    /* Does the user want the connection rate back? If so give it to them. */
    if(conn_rate) {
        *conn_rate = modem_get_connection_rate();
    }

    dbglog(DBG_KDEBUG, "ppp_modem: connected at %lu bps\n",
           modem_get_connection_rate());

    /* We connected to the peer successfully, set our device with libppp. */
    ppp_set_device(&modem_dev);

    return 0;
}
