/* KallistiOS ##version##

   hardware/scif.c
   Copyright (C)2000,2001,2004 Megan Potter
*/

#include <stdio.h>
#include <errno.h>
#include <kos/dbgio.h>
#include <arch/arch.h>
#include <arch/spinlock.h>
#include <arch/irq.h>
#include <dc/fs_dcload.h>

/*

This module handles very basic serial I/O using the SH4's SCIF port. FIFO
mode is used by default; you can turn this off to avoid forcing a wait
when there is no serial device attached.

Unlike in KOS 1.x, this is not designed to be used as the normal I/O, but
simply as an early debugging device in case something goes wrong in the
kernel or for debugging it.

*/

/* SCIF registers */
#define SCIFREG08(x) *((volatile uint8 *)(x))
#define SCIFREG16(x) *((volatile uint16 *)(x))
#define SCSMR2  SCIFREG16(0xffe80000)
#define SCBRR2  SCIFREG08(0xffe80004)
#define SCSCR2  SCIFREG16(0xffe80008)
#define SCFTDR2 SCIFREG08(0xffe8000C)
#define SCFSR2  SCIFREG16(0xffe80010)
#define SCFRDR2 SCIFREG08(0xffe80014)
#define SCFCR2  SCIFREG16(0xffe80018)
#define SCFDR2  SCIFREG16(0xffe8001C)
#define SCSPTR2 SCIFREG16(0xffe80020)
#define SCLSR2  SCIFREG16(0xffe80024)

/* Default serial parameters */
static int serial_baud = DEFAULT_SERIAL_BAUD,
           serial_fifo = DEFAULT_SERIAL_FIFO;

/* This will get set to zero if we fail to send. */
static int serial_enabled = 1;

/* Set serial parameters; this is not platform independent like I want
   it to be, but it should be generic enough to be useful. */
void scif_set_parameters(int baud, int fifo) {
    serial_baud = baud;
    serial_fifo = fifo;
}

/* Receive ring buffer */
#define BUFSIZE 1024
static uint8 recvbuf[BUFSIZE];
static int rb_head = 0, rb_tail = 0, rb_cnt = 0;
static int rb_paused = 0;

static void rb_reset(void) {
    rb_head = rb_tail = rb_cnt = rb_paused = 0;
}

static void rb_push_char(int c) {
    recvbuf[rb_head] = c;
    rb_head = (rb_head + 1) % BUFSIZE;
    rb_cnt++;

    /* If we're within 32 bytes of being out of space, pause for
       the moment. */
    if(!rb_paused && (BUFSIZE - rb_cnt) < 32) {
        rb_paused = 1;
        SCSPTR2 = 0x20;     /* Set CTS=0 */
    }
}

static int rb_pop_char(void) {
    int c;
    c = recvbuf[rb_tail];
    rb_tail = (rb_tail + 1) % BUFSIZE;
    rb_cnt--;

    /* If we're paused and clear again, re-enabled receiving. */
    if(rb_paused && (BUFSIZE - rb_cnt) >= 64) {
        rb_paused = 0;
        SCSPTR2 = 0x00;
    }

    return c;
}

/* static int rb_space_free(void) {
    return BUFSIZE - rb_cnt;
} */

static int rb_space_used(void) {
    return rb_cnt;
}


/* Serial receive and receive error interrupts. When this is triggered we
   must look for available data and error conditions, and clear them all
   out if possible. If our internal ring buffer comes close to overflowing,
   the best we can do is twiddle RTS/CTS for a while. */
static void scif_err_irq(irq_t src, irq_context_t *cxt, void *data) {
    (void)src;
    (void)cxt;
    (void)data;

    /* Clear status bits */
    SCSCR2 &= ~0x08;
    SCSCR2 |= 0x08;

    printf("scif_err_irq called\n");

    /* Did we get an error condition? */
    if(SCFSR2 & 0x9c) {     /* Check ER, BRK, FER, PER */
        printf("SCFSR2 status was %04x\n", SCFSR2);
        /* Try to clear it */
        SCFCR2 = 0x06;
        SCFCR2 = 0x88;
        SCFSR2 &= ~0x9c;
    }

    if(SCLSR2 & 0x01) {     /* ORER */
        printf("SCLSR2 status was %04x\n", SCLSR2);
        /* Try to clear it */
        SCFCR2 = 0x06;
        SCFCR2 = 0x88;
        SCLSR2 = 0x00;
    }
}

static void scif_data_irq(irq_t src, irq_context_t *cxt, void *data) {
    (void)src;
    (void)cxt;
    (void)data;

    /* Clear status bits */
    SCSCR2 &= ~0x40;
    SCSCR2 |= 0x40;

    /* Check for received data available. */
    if(SCFSR2 & 3) {
        while(SCFDR2 & 0x1f) {
            int c = SCFRDR2;
            rb_push_char(c);
        }

        SCFSR2 &= ~3;
    }
}

/* Are we using IRQs? */
static int scif_irq_usage = 0;
int scif_set_irq_usage(int on) {
    scif_irq_usage = on;

    /* Clear out the buffer in any case */
    rb_reset();

    if(scif_irq_usage) {
        /* Hook the SCIF interrupt */
        irq_set_handler(EXC_SCIF_ERI, scif_err_irq, NULL);
        irq_set_handler(EXC_SCIF_BRI, scif_err_irq, NULL);
        irq_set_handler(EXC_SCIF_RXI, scif_data_irq, NULL);
        irq_set_priority(IRQ_SRC_SCIF, 14);

        /* Enable transmit/receive, recv/recv error ints */
        SCSCR2 |= 0x48;
    }
    else {
        /* Disable transmit/receive, recv/recv error ints */
        SCSCR2 &= ~0x48;

        /* Unhook the SCIF interrupt */
        irq_set_priority(IRQ_SRC_SCIF, IRQ_PRIO_MASKED);
        irq_set_handler(EXC_SCIF_ERI, NULL, NULL);
        irq_set_handler(EXC_SCIF_BRI, NULL, NULL);
        irq_set_handler(EXC_SCIF_RXI, NULL, NULL);
    }

    return 0;
}

/* We are always detected, though we might end up realizing there's no
   cable connected later... */
int scif_detected(void) {
    return 1;
}

/* We use this for the dbgio interface because we always init SCIF. */
int scif_init_fake(void) {
    return 0;
}

/* Initialize the SCIF port; */
/* recv trigger to 1 byte */
int scif_init(void) {
    int i;
    unsigned char scbrr2 = 0;
    unsigned short scsmr2 = 0;

    /*  If dcload-serial is active, then do nothing here, or we'll
        screw that up. */
    if(dcload_type == DCLOAD_TYPE_SER)
        return 0;

    /* Disable interrupts, transmit/receive,
       and use internal or external clock */
    SCSCR2 = serial_baud ? 0x0 : 0x02;

    /* Enter reset mode */
    SCFCR2 = 0x06;

    if(serial_baud > 9600) {
        scsmr2 = 0;
        scbrr2 = (50000000 / (32 * serial_baud)) - 1;
    }
    else if(serial_baud > 4800) {
        scsmr2 = 1;
        scbrr2 = (50000000 / (128 * serial_baud)) - 1;
    }
    else if(serial_baud > 600) {
        scsmr2 = 2;
        scbrr2 = (50000000 / (512 * serial_baud)) - 1;
    }
    else if(serial_baud) {
        scsmr2 = 3;
        scbrr2 = (50000000 / (2048 * serial_baud)) - 1;
    }

    /* 8N1, clock as set above (scsmr2) */
    SCSMR2 = scsmr2;

    /* Set baudrate, N = P0/(32*B)-1 */
    /* B = P0/32*(N+1) */
    SCBRR2 = scbrr2;

    /* Wait a bit for it to stabilize */
    for(i = 0; i < 800000; i++)
        __asm__("nop");

    /* Unreset, disable hardware flow control, triggers on 8 bytes */
    SCFCR2 = 0x40;

    /* Disable manual pin control */
    SCSPTR2 = 0;

    /* Clear status */
    (void)SCFSR2;
    SCFSR2 = 0x60;
    (void)SCLSR2;
    SCLSR2 = 0;

    /* Enable transmit/receive */
    /* If external clock is used set CKE1 bit */
    SCSCR2 = serial_baud ? 0x30 : 0x32;

    /* Wait a bit for it to stabilize */
    for(i = 0; i < 800000; i++)
        __asm__("nop");

    return 0;
}

int scif_shutdown(void) {
    scif_set_irq_usage(DBGIO_MODE_POLLED);
    return 0;
}

/* Read one char from the serial port (-1 if nothing to read) */
int scif_read(void) {
    if(!serial_enabled) {
        errno = EIO;
        return -1;
    }

    if(scif_irq_usage) {
        /* Do we have anything ready? */
        if(rb_space_used() <= 0) {
            errno = EAGAIN;
            return -1;
        }
        else
            return rb_pop_char();
    }
    else {
        int c;

        if(!(SCFDR2 & 0x1f)) {
            errno = EAGAIN;
            return -1;
        }

        /* Get the input char */
        c = SCFRDR2;

        /* Ack */
        SCFSR2 &= ~0x92;

        return c;
    }
}

/* Write one char to the serial port (call serial_flush()!) */
int scif_write(int c) {
    int timeout = 800000;

    if(!serial_enabled) {
        errno = EIO;
        return -1;
    }

    /* Wait until the transmit buffer has space. Too long of a failure
       is indicative of no serial cable. */
    while(!(SCFSR2 & 0x20) && timeout > 0)
        timeout--;

    if(timeout <= 0) {
        serial_enabled = 0;
        errno = EIO;
        return -1;
    }

    /* Send the char */
    SCFTDR2 = c;

    /* Clear status */
    SCFSR2 &= 0xff9f;

    return 1;
}

/* Flush all FIFO'd bytes out of the serial port buffer */
int scif_flush(void) {
    int timeout = 800000;

    if(!serial_enabled) {
        errno = EIO;
        return -1;
    }

    SCFSR2 &= 0xbf;

    while(!(SCFSR2 & 0x40) && timeout > 0)
        timeout--;

    if(timeout <= 0) {
        serial_enabled = 0;
        errno = EIO;
        return -1;
    }

    SCFSR2 &= 0xbf;

    return 0;
}

/* Send an entire buffer */
int scif_write_buffer(const uint8 *data, int len, int xlat) {
    int rv, i = 0, c;

    while(len-- > 0) {
        c = *data++;

        if(xlat) {
            if(c == '\n') {
                if(scif_write('\r') < 0)
                    return -1;

                i++;
            }
        }

        rv = scif_write(c);

        if(rv < 0)
            return -1;

        i += rv;
    }

    if(scif_flush() < 0)
        return -1;

    return i;
}

/* Read an entire buffer (block) */
int scif_read_buffer(uint8 *data, int len) {
    int c, i = 0;

    while(len-- > 0) {
        while((c = scif_read()) == -1) {
            if(errno != EAGAIN)
                return -1;
        }

        *data++ = c;
        i++;
    }

    return i;
}

/* Tie all of that together into a dbgio package. */
dbgio_handler_t dbgio_scif = {
    "scif",
    scif_detected,
    scif_init_fake,
    scif_shutdown,
    scif_set_irq_usage,
    scif_read,
    scif_write,
    scif_flush,
    scif_write_buffer,
    scif_read_buffer
};
