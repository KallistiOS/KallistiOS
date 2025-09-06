/* KallistiOS ##version##

   libppp/ppp_scif.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <stdio.h>
#include <stdint.h>

#include <kos/dbglog.h>

#include <dc/scif.h>
#include <dc/fs_dcload.h>

#include <ppp/ppp.h>

static int ppp_scif_detect_init(ppp_device_t *self) {
    (void)self;

    return 0;
}

static int ppp_scif_shutdown(ppp_device_t *self) {
    (void)self;

    return scif_shutdown();
}

static int ppp_scif_tx(ppp_device_t *self, const uint8_t *data, size_t len,
                       uint32_t flags) {
    size_t i;
    (void)self;
    (void)flags;

    for(i = 0; i < len; ++i) {
        scif_write(data[i]);
    }

    if(flags & PPP_TX_END_OF_PKT)
        scif_flush();

    return 0;
}

static const uint8_t *ppp_scif_rx(ppp_device_t *self, ssize_t *out_len) {
    static uint8_t rb[1024];
    int rc;
    ssize_t cnt = 0;

    (void)self;

    /* Read anything that's waiting. */
    while(cnt < 1024 && (rc = scif_read()) != -1) {
        rb[cnt++] = (uint8_t)rc;
    }

    /* If we got anything, return what we got. */
    if(cnt) {
        *out_len = cnt;
        return rb;
    }

    *out_len = 0;
    return NULL;
}

static ppp_device_t scif_dev = {
    "scif",                             /* name */
    "PPP over Dreamcast Serial Port",   /* descr */
    0,                                  /* index */
    0,                                  /* flags */
    NULL,                               /* privdata */
    &ppp_scif_detect_init,              /* detect */
    &ppp_scif_detect_init,              /* init */
    &ppp_scif_shutdown,                 /* shutdown */
    &ppp_scif_tx,                       /* tx */
    &ppp_scif_rx                        /* rx */
};

int ppp_scif_init(int bps) {
    /* Make sure we're not using dcload-serial. If we are, we really shouldn't
       take over the serial port from it. */
    if(dcload_type == DCLOAD_TYPE_SER) {
        dbglog(DBG_KDEBUG, "ppp_scif_init: aborting -- using dcload-serial.\n");
        return -1;
    }

    /* Initialize the serial port. */
    scif_set_parameters(bps, 1);
    scif_init();

    /* Clear any bytes that may already have been transmitted to us and enable
       buffered receives. */
    while(scif_read() != -1) ;
    scif_set_irq_usage(1);
    while(scif_read() != -1) ;

    /* Set our device with libppp. */
    ppp_set_device(&scif_dev);

    return 0;
}
