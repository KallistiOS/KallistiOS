/* KallistiOS ##version##

   hardware/scif-spi.c
   Copyright (C) 2012 Lawrence Sebald
   Copyright (C) 2023 Ruslan Rostovtsev
   Copyright (C) 2024 Paul Cercueil
*/

#include <dc/scif.h>
#include <dc/fs_dcload.h>
#include <arch/timer.h>
#include <kos/dbglog.h>
#include <kos/regfield.h>

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

/* Values for the SCSPTR2 register */
#define PTR2_RTSIO  BIT(7)
#define PTR2_RTSDT  BIT(6)
#define PTR2_CTSIO  BIT(5)
#define PTR2_CTSDT  BIT(4)
#define PTR2_SPB2IO BIT(1)
#define PTR2_SPB2DT BIT(0)

/* This doesn't seem to actually be necessary on any of the SD cards I've tried,
   but I'm keeping it around, just in case... */
#define SD_WAIT() __asm__("nop\n\tnop\n\tnop\n\tnop\n\tnop")

static uint16 scsptr2 = 0;

/* Re-initialize the state of SCIF to match what we need for communication with
   the SPI device. We basically take complete control of the pins of the port
   directly, overriding the normal byte FIFO and whatnot. */
int scif_spi_init(void) {
    /* Make sure we're not using dcload-serial. If we are, then we definitely do
       not have a SPI device on the serial port. */
    if(*DCLOADMAGICADDR == DCLOADMAGICVALUE && dcload_type == DCLOAD_TYPE_SER) {
        dbglog(DBG_KDEBUG, "scif_spi_init: no spi device -- using "
               "dcload-serial\n");
        return -1;
    }

    /* Clear most of the registers, since we're going to do all the hard work in
       software anyway... */
    SCSCR2 = 0;
    SCFCR2 = 0x06;                          /* Empty the FIFOs */
    SCFCR2 = 0;
    SCSMR2 = 0;
    SCFSR2 = 0;
    SCLSR2 = 0;
    SCSPTR2 = scsptr2 = PTR2_RTSIO | PTR2_RTSDT | PTR2_CTSIO | PTR2_SPB2IO;

    return 0;
}

int scif_spi_shutdown(void) {
    return 0;
}

void scif_spi_set_cs(int v) {
    if(v)
        scsptr2 |= PTR2_RTSDT;
    else
        scsptr2 &= ~PTR2_RTSDT;
    SCSPTR2 = scsptr2;
}

uint8 scif_spi_rw_byte(uint8 b) {
    uint16 tmp = scsptr2 & ~PTR2_CTSDT & ~PTR2_SPB2DT;
    uint8 bit;
    uint8 rv = 0;

    /* Write the data out, one bit at a time (most significant bit first), while
       reading in a data byte, one bit at a time as well...
       For some reason, we have to have the bit set on the Tx line before we set
       CTS, otherwise it doesn't work -- that's why this looks so ugly... */
    SCSPTR2 = tmp | (bit = (b >> 7) & 0x01);    /* write 7 */
    SD_WAIT();
    SCSPTR2 = tmp | bit | PTR2_CTSDT;
    rv = SCSPTR2 & PTR2_SPB2DT;                 /* read 7 */
    SCSPTR2 = tmp | (bit = (b >> 6) & 0x01);    /* write 6 */
    SD_WAIT();
    SCSPTR2 = tmp | bit | PTR2_CTSDT;
    rv = (rv << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* read 6 */
    SCSPTR2 = tmp | (bit = (b >> 5) & 0x01);    /* write 5 */
    SD_WAIT();
    SCSPTR2 = tmp | bit | PTR2_CTSDT;
    rv = (rv << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* read 5 */
    SCSPTR2 = tmp | (bit = (b >> 4) & 0x01);    /* write 4 */
    SD_WAIT();
    SCSPTR2 = tmp | bit | PTR2_CTSDT;
    rv = (rv << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* read 4 */
    SCSPTR2 = tmp | (bit = (b >> 3) & 0x01);    /* write 3 */
    SD_WAIT();
    SCSPTR2 = tmp | bit | PTR2_CTSDT;
    rv = (rv << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* read 3 */
    SCSPTR2 = tmp | (bit = (b >> 2) & 0x01);    /* write 2 */
    SD_WAIT();
    SCSPTR2 = tmp | bit | PTR2_CTSDT;
    rv = (rv << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* read 2 */
    SCSPTR2 = tmp | (bit = (b >> 1) & 0x01);    /* write 1 */
    SD_WAIT();
    SCSPTR2 = tmp | bit | PTR2_CTSDT;
    rv = (rv << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* read 1 */
    SCSPTR2 = tmp | (bit = (b >> 0) & 0x01);    /* write 0 */
    SD_WAIT();
    SCSPTR2 = tmp | bit | PTR2_CTSDT;
    rv = (rv << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* read 0 */

    return rv;
}

/* Very accurate 1.5usec delay... */
static void slow_rw_delay(void) {
    timer_spin_delay_ns(1500);
}

uint8 scif_spi_slow_rw_byte(uint8 b) {
    int i;
    uint8 rv = 0;
    uint16 tmp = scsptr2 & ~PTR2_CTSDT & ~PTR2_SPB2DT;
    uint8 bit;

    for(i = 7; i >= 0; --i) {
        SCSPTR2 = tmp | (bit = (b >> i) & 0x01);
        slow_rw_delay();
        SCSPTR2 = tmp | bit | PTR2_CTSDT;
        rv = (rv << 1) | (SCSPTR2 & PTR2_SPB2DT);
        slow_rw_delay();
    }

    return rv;
}

void scif_spi_write_byte(uint8 b) {
    uint16 tmp = scsptr2 & ~PTR2_CTSDT & ~PTR2_SPB2DT;
    uint8 bit;

    /* Write the data out, one bit at a time (most significant bit first)...
       For some reason, we have to have the bit set on the Tx line before we set
       CTS, otherwise it doesn't work -- that's why this looks so ugly... */
    SCSPTR2 = tmp | (bit = (b >> 7) & 0x01);
    SCSPTR2 = tmp | bit | PTR2_CTSDT;
    SD_WAIT();
    SCSPTR2 = tmp | (bit = (b >> 6) & 0x01);
    SCSPTR2 = tmp | bit | PTR2_CTSDT;
    SD_WAIT();
    SCSPTR2 = tmp | (bit = (b >> 5) & 0x01);
    SCSPTR2 = tmp | bit | PTR2_CTSDT;
    SD_WAIT();
    SCSPTR2 = tmp | (bit = (b >> 4) & 0x01);
    SCSPTR2 = tmp | bit | PTR2_CTSDT;
    SD_WAIT();
    SCSPTR2 = tmp | (bit = (b >> 3) & 0x01);
    SCSPTR2 = tmp | bit | PTR2_CTSDT;
    SD_WAIT();
    SCSPTR2 = tmp | (bit = (b >> 2) & 0x01);
    SCSPTR2 = tmp | bit | PTR2_CTSDT;
    SD_WAIT();
    SCSPTR2 = tmp | (bit = (b >> 1) & 0x01);
    SCSPTR2 = tmp | bit | PTR2_CTSDT;
    SD_WAIT();
    SCSPTR2 = tmp | (bit = (b >> 0) & 0x01);
    SCSPTR2 = tmp | bit | PTR2_CTSDT;
    SD_WAIT();
    SCSPTR2 = tmp;
}

uint8 scif_spi_read_byte(void) {
    uint8 b = 0xff;
    uint16 tmp = (scsptr2 & ~PTR2_CTSDT) | PTR2_SPB2DT;

    /* Read the data in, one bit at a time (most significant bit first) */
    SCSPTR2 = tmp;
    SCSPTR2 = tmp | PTR2_CTSDT;
    b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* read 7 */
    SCSPTR2 = tmp;
    SCSPTR2 = tmp | PTR2_CTSDT;
    b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* read 6 */
    SCSPTR2 = tmp;
    SCSPTR2 = tmp | PTR2_CTSDT;
    b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* read 5 */
    SCSPTR2 = tmp;
    SCSPTR2 = tmp | PTR2_CTSDT;
    b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* read 4 */
    SCSPTR2 = tmp;
    SCSPTR2 = tmp | PTR2_CTSDT;
    b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* read 3 */
    SCSPTR2 = tmp;
    SCSPTR2 = tmp | PTR2_CTSDT;
    b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* read 2 */
    SCSPTR2 = tmp;
    SCSPTR2 = tmp | PTR2_CTSDT;
    b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* read 1 */
    SCSPTR2 = tmp;
    SCSPTR2 = tmp | PTR2_CTSDT;
    b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* read 0 */

    return b;
}


void scif_spi_read_data(uint8 *buffer, size_t len) {
    uint8 b = 0xff;
    uint16 tmp;
    uint32 data;
    uint32 *ptr;

    /* Less optimized version for unaligned buffers or lengths not divisible by
       four. */
    if((((uint32)buffer) & 0x03) || (len & 0x03)) {
        while(len--) {
            *buffer++ = scif_spi_read_byte();
        }

        return;
    }

    b = 0xff;
    tmp = (scsptr2 & ~PTR2_CTSDT) | PTR2_SPB2DT;
    ptr = (uint32 *)buffer;
    SCSPTR2 = tmp;

    for(; len > 0; len -= 4) {
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 7 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 6 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 5 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 4 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 3 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 2 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 1 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 0 */
        SCSPTR2 = tmp;
        data = b;

        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 7 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 6 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 5 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 4 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 3 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 2 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 1 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 0 */
        SCSPTR2 = tmp;
        data |= b << 8;

        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 7 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 6 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 5 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 4 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 3 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 2 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 1 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 0 */
        SCSPTR2 = tmp;
        data |= b << 16;

        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 7 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 6 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 5 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 4 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 3 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 2 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 1 */
        SCSPTR2 = tmp;
        SCSPTR2 = tmp | PTR2_CTSDT;
        b = (b << 1) | (SCSPTR2 & PTR2_SPB2DT);   /* 0 */
        SCSPTR2 = tmp;
        data |= b << 24;
        *ptr++ = data;
    }
}
