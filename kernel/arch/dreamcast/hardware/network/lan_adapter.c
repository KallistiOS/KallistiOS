/* KallistiOS ##version##

   net/lan_adapter.c

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2010 Lawrence Sebald

*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <dc/g2bus.h>
#include <dc/asic.h>
#include <dc/flashrom.h>
#include <dc/net/lan_adapter.h>
#include <arch/irq.h>
#include <kos/dbglog.h>
#include <kos/net.h>
#include <kos/thread.h>

/*

   Contains a low-level ethernet driver for the "Lan Adapter" (HIT-0300), which
   is a Fujitsu ethernet chip attached to the 8-bit interface of the DC's
   "parallel port". The block diagram looks something like this:

   The DC Lan Adapter is an 8-bit G2 peripheral with the following block
   diagram:

   G2 <---> Altera PLD <---> MB86967 Ethernet <---> TDK "Pulse Transformer"
                                    ^                         ^
                                    |                         |
                                    v                         v
                              M5M5278DVP RAM            10BT ethernet

   Its IO base is located at 0xa060xxxx, like the modem, and the register
   interface seems very similar (I suspect they actually use the exact same
   PLD in both, to save money).

   As for the Fujitsu ethernet chip, the latest updated driver in BSD was
   for FreeBSD 2.x (!!) and was for PCMCIA, so a bit of interpolation was
   required. Fortunately the Fujitsu chip is _much_ simpler and easier to
   work with than the later RTL chip, but unfortunately, that simplicity
   comes at a cost. Each packet to be send or received must be processed
   over a PIO interface one byte at a time. This is probably why Sega dumped
   it for the RTL design later on, because you can imagine the system load
   to do that while trying to process 3D graphics and such...

   This driver is really simplistic, but this should form the basis with
   which a better one can be written if necessary. It was developed using
   KOS CVS (1.1.5+).

   If anyone has a DC Lan Adapter which this doesn't work with, please let
   me know!

   Thanks to TheGypsy for test hardware.

   Historical note: this driver was adapted from the dc-load-ip modifications
   I made for dc-load-ip-la.

*/

/*
   Register names. The first set of register names is always valid; the
   second depends on which bank you have selected. During normal
   operation, the last bank will be selected (the one containing the
   BMPR regs). Source: Fujitsu PDF.
*/

/* Bank-agnostic registers; the bank for these is specified as -1 so
   that the register access function will ignore it */
#define DLCR0R  0
#define DLCR0   DLCR0R,-1   /* Transmit Status Register */
#define DLCR0_TMTOK (1<<7)      /* Transmit OK */
#define DLCR0_NETBSY    (1<<6)      /* Net Busy (carrier detected) */
#define DLCR0_TMTREC    (1<<5)      /* Transmit Packet Receive */
#define DLCR0_SRTPKT    (1<<4)      /* Short Packet */
#define DLCR0_JABBER    (1<<3)      /* Jabber */
#define DLCR0_COL   (1<<2)      /* Collision Error */
#define DLCR0_16COL (1<<1)      /* 16 Collision Error */
#define DLCR0_BUSWRERR  (1<<0)      /* Bus Write Error */
#define DLCR0_CLEARALL  0xff

#define DLCR1R  1
#define DLCR1   DLCR1R,-1   /* Receive Status Register */
#define DLCR1_PKTRDY    (1<<7)      /* Packet ready */
#define DLCR1_BUSRDERR  (1<<6)      /* Bus Read Error */
#define DLCR1_DMAEOP    (1<<5)      /* DMA Completed */
#define DLCR1_RMTRST    (1<<4)      /* Remote Reset */
#define DLCR1_RXSRTPKT  (1<<3)      /* Short Packet */
#define DLCR1_ALGERR    (1<<2)      /* Alignment Error */
#define DLCR1_CRCERR    (1<<1)      /* CRC Error */
#define DLCR1_OVRFLO    (1<<0)      /* Overflow Error */
#define DLCR1_CLEARALL  0xff

#define DLCR2R  2
#define DLCR2   DLCR2R,-1   /* Transmit Interrupt Enable Register */
/* Use DLCR0 Constants */

#define DLCR3R  3
#define DLCR3   DLCR3R,-1   /* Receive Interrupt Enable Register */
/* Use DLCR1 Constants */

#define DLCR4R  4
#define DLCR4   DLCR4R,-1   /* Transmit Mode Register */
#define DLCR4_COLCNT(x) (((x) >> 4) & 0x0f) /* Read collision count (ro) */
#define DLCR4_CNTRL (1<<2)      /* DREQ Control */
#define DLCR4_LBC   (1<<1)      /* Loopback Control */
#define DLCR4_DSC   (1<<0)      /* Disable Carrier Detect */

#define DLCR5R  5
#define DLCR5   DLCR5R,-1   /* Receive Mode Register */
#define DLCR5_BUFEMP    (1<<6)      /* Buffer empty? (ro) */
#define DLCR5_ACPTBADPKT (1<<5)     /* Accept packets with bad CRC/length */
#define DLCR5_ADDSIZE   (1<<4)      /* Mac Address Size */
#define DLCR5_ENASRTPKT (1<<3)      /* Enable Short Packet Receive */
#define DLCR5_AM_OFF    0       /* Address mode: receive nothing */
#define DLCR5_AM_SELF   1       /* "          ": only our packets */
#define DLCR5_AM_OTHER  2       /* "          ": only other node packets */
#define DLCR5_AM_PROM   3       /* "          ": receive ALL packets */
#define DLCR5_AM_MASK   3

#define DLCR6R  6
#define DLCR6   DLCR6R,-1   /* Control Register 1 */
#define DLCR6_DLCRST    (1<<7)      /* Reset the DLC and buffers */
#define DLCR6_SCTS  (1<<6)      /* SRAM cycle time select */
#define DLCR6_SBW   (1<<5)      /* System Bus Width Select: 16/8 bit */
#define DLCR6_TBS_2K    0       /* TX Buffer Size: 2K, 1-bank */
#define DLCR6_TBS_4K    4       /* TX Buffer Size: 4K, 2-bank */
#define DLCR6_TBS_8K    8       /* TX Buffer Size: 8K, 2-bank */
#define DLCR6_TBS_16K   16      /* TX Buffer Size: 16K, 2-bank */
#define DLCR6_BS    (1<<1)      /* External Buffer Size: 8K/32K */

#define DLCR7R  7
#define DLCR7   DLCR7R,-1   /* Control Register 2 */
#define DLCR7_IDENT(x)  (((x) >> 6) & 3) /* Chip Identification (ro) */
#define DLCR7_ID_MB86960A   0
#define DLCR7_ID_MB86964    1
#define DLCR7_ID_MB86967    2
#define DLCR7_ID_MB86965A   3
#define DLCR7_NSTBY (1<<6)      /* Not-standby mode */
#define DLCR7_RBS_D8    0       /* Read Bank Select: DLCR8 - DLCR15 */
#define DLCR7_RBS_M8    4       /* "              ": MAR8 - MAR15 */
#define DLCR7_RBS_B8    8       /* "              ": BMPR8 - BMPR15 */
#define DLCR7_RBS_MASK  0x0C
#define DLCR7_EOP   (1<<1)      /* DMA End Signal: active LOW / HIGH */
#define DLCR7_BSWAP (1<<0)      /* Byte Swap */

#define BMPR16  16,-1       /* EEPROM Control */
#define BMPR16_ESK  (1<<6)      /* EEPROM Shift Clocks */
#define BMPR16_ECS  (1<<5)      /* EEPROM Chip Select */
#define BMPR17  17,-1       /* EEPROM Data */
#define BMPR17_EDIO (1<<7)      /* EEPROM Data I/O Bit */

/* The rest of these registers must be accessed using the banking
   mechanism; i.e., their bank must be selected before they can be
   accessed at all. */
#define DLCR8   8,DLCR7_RBS_D8  /* Node ID (Mac Address) Registers */

#define DLCR14  14,DLCR7_RBS_D8 /* TDR Low */

#define DLCR15  15,DLCR7_RBS_D8 /* TDR High */

#define MAR8    8,DLCR7_RBS_M8  /* Multicast Address Registers */

#define BMPR8   8,DLCR7_RBS_B8  /* Buffer Memory Port */

#define BMPR10  10,DLCR7_RBS_B8 /* Transmit Packet Count */
#define BMPR10_TX   (1<<7)      /* Start transmit */
#define BMPR10_PKTCNT(x) ((x) & 0x7f)   /* Remaining packet count */

#define BMPR11  11,DLCR7_RBS_B8 /* 16 Collision Control */
#define BMPR11_AUTO 6       /* Auto-retransmit */
#define BMPR11_SKIP 7       /* Skip problem packet */
#define BMPR11_MANUAL   2       /* Manually handle 16-collisions */
#define BMPR11_MANUAL_DISCARD   3   /* Discard packet during manual handling */

#define BMPR12  12,DLCR7_RBS_B8 /* DMA Enable Register, or as Fujitsu says, the DAM enable ;-) */
#define BMPR12_NLPR (1<<3)      /* Long Packet Receive Disable */
#define BMPR12_RENA (1<<1)      /* Receive DMA Enable */
#define BMPR12_TENA (1<<0)      /* Transmit DMA Enable */

#define BMPR13  13,DLCR7_RBS_B8 /* DMA Burst/Transceiver Mode Control */
#define BMPR13_LTA  (1<<5)      /* Link Test Enable */
#define BMPR13_DB_1 0       /* DMA Burst Cycle Count: 1 */
#define BMPR13_DB_4 1       /* "                   ": 4 */
#define BMPR13_DB_18    2       /* "                   ": 18 */
#define BMPR13_DB_12    3       /* "                   ": 12 */

#define BMPR14  14,DLCR7_RBS_B8 /* Receiver Control/Transceiver IRQ Control */
#define BMPR14_LKFE (1<<6)      /* Link Failure Interrupt Enable */
#define BMPR14_SDM  (1<<5)      /* Shutdown mode */
#define BMPR14_SKIPRX   (1<<2)      /* Skip RX Packet (r/w) */
#define BMPR14_SQE  (1<<1)      /* Signal Quality Error Int Enable */
#define BMPR14_FS   (1<<0)      /* Filter Self (in promisc mode) */

#define BMPR15  15,DLCR7_RBS_B8 /* Transceiver Status */
#define BMPR15_LKF  (1<<6)      /* Link failure */
#define BMPR15_POLREV   (1<<5)      /* Polarity Reverse */
#define BMPR15_SQE  (1<<1)      /* Signal Quality Error */
#define BMPR15_CLEARALL (BMPR15_LKF | BMPR15_POLREV | BMPR15_SQE)

#define REGLOC(x)   (0xa0600400 + (x) * 4)
#define G2_8BP_RST  0xa0600480

/* What bank do we have selected now? */
static int la_bank = -1;

/* Our network struct */
netif_t la_if;

/* What's our current state? */
#define LA_NOT_STARTED      0
#define LA_DETECTED     1
#define LA_RUNNING      2
#define LA_PAUSED       3
static int la_started = LA_NOT_STARTED;

/* Mac address (read from EEPROM) */
static uint8 la_mac[6];

/* Forward declaration */
static void la_irq_hnd(uint32 code, void *data);

/* Set the current bank */
static void la_set_bank(int bank) {
    int i;

    if(la_bank == bank)
        return;

    i = g2_read_32(REGLOC(DLCR7R)) & 0xff;
    g2_write_8(REGLOC(DLCR7R), (i & ~DLCR7_RBS_MASK) | bank);
    la_bank = bank;
}

/* Read a register */
static int la_read(int reg, int bank) {
    if(bank != -1)
        la_set_bank(bank);

    return g2_read_32(REGLOC(reg)) & 0xff;
}

/* Write a register */
static void la_write(int reg, int bank, int value) {
    if(bank != -1)
        la_set_bank(bank);

    g2_write_8(REGLOC(reg), value);
}

/* This is based on the JLI EEPROM reader from FreeBSD. EEPROM in the
   Sega adapter is a bit simpler than what is described in the Fujitsu
   manual -- it appears to contain only the MAC address and not a base
   address like the manual says. EEPROM is read one bit (!) at a time
   through the EEPROM interface port. */
static void la_strobe_eeprom(void) {
    la_write(BMPR16, BMPR16_ECS);
    la_write(BMPR16, BMPR16_ECS | BMPR16_ESK);
    la_write(BMPR16, BMPR16_ECS | BMPR16_ESK);
    la_write(BMPR16, BMPR16_ECS);
}
static void la_read_eeprom(uint8 *data) {
    uint8 val;
    int n, bit;

    /* Read bytes from EEPROM, two per iteration */
    for(n = 0; n < 3; n++) {
        /* Reset the EEPROM interface */
        la_write(BMPR16, 0);
        la_write(BMPR17, 0);

        /* Start EEPROM access */
        la_write(BMPR16, BMPR16_ECS);
        la_write(BMPR17, BMPR17_EDIO);
        la_strobe_eeprom();

        /* Pass the iteration count as well as a READ command */
        val = 0x80 | n;

        for(bit = 0x80; bit != 0x00; bit >>= 1) {
            la_write(BMPR17, (val & bit) ? BMPR17_EDIO : 0);
            la_strobe_eeprom();
        }

        la_write(BMPR17, 0);

        /* Read a byte */
        val = 0;

        for(bit = 0x80; bit != 0x00; bit >>= 1) {
            la_strobe_eeprom();

            if(la_read(BMPR17) & BMPR17_EDIO)
                val |= bit;
        }

        *data++ = val;

        /* Read one more byte */
        val = 0;

        for(bit = 0x80; bit != 0x00; bit >>= 1) {
            la_strobe_eeprom();

            if(la_read(BMPR17) & BMPR17_EDIO)
                val |= bit;
        }

        *data++ = val;
    }
}

/* Reset the lan adapter and verify that it's there and alive */
static int la_detect(void) {
    int type;

    assert_msg(la_started == LA_NOT_STARTED, "la_detect called out of sequence");

    /* Reset the interface */
    g2_write_8(G2_8BP_RST, 0);
    g2_write_8(G2_8BP_RST, 1);
    g2_write_8(G2_8BP_RST, 0);

    /* Give it a few ms to come back */
    thd_sleep(100);

    /* Read the chip type and verify it */
    type = DLCR7_IDENT(la_read(DLCR7));

    if(type != DLCR7_ID_MB86967) {
        return -1;
    }

    /* That should do */
    la_started = LA_DETECTED;
    return 0;
}

/* Reset the lan adapter and set it up for send/receive */
static int la_hw_init(void) {
    int i;

    assert_msg(la_started == LA_DETECTED, "la_hw_init called out of sequence");

    /* Clear interrupt status */
    la_write(DLCR0, DLCR0_CLEARALL);
    la_write(DLCR1, DLCR0_CLEARALL);

    /* Power down chip */
    thd_sleep(2);
    la_write(DLCR7, la_read(DLCR7) & ~DLCR7_NSTBY);
    thd_sleep(2);

    /* Reset Data Link Control */
    thd_sleep(2);
    la_write(DLCR6, la_read(DLCR6) | DLCR6_DLCRST);
    thd_sleep(2);

    /* Power up the chip */
    thd_sleep(2);
    la_write(DLCR7, la_read(DLCR7) | DLCR7_NSTBY);
    thd_sleep(2);

    /* Read the EEPROM data */
    la_read_eeprom(la_mac);

    /* Write the MAC address into the Node ID regs */
    for(i = 0; i < 6; i++)
        la_write(i + DLCR8, la_mac[i]);

    /* Clear the multicast address */
    for(i = 0; i < 8; i++)
        la_write(i + MAR8, 0);

    dbglog(DBG_DEBUG, "lan_adapter: MAC address is %02x:%02x:%02x:%02x:%02x:%02x\n",
           la_mac[0], la_mac[1], la_mac[2],
           la_mac[3], la_mac[4], la_mac[5]);

    /* Setup the BMPR bank for normal operation */
    la_write(BMPR10, 0);
    la_write(BMPR11, BMPR11_AUTO);
    la_write(BMPR12, 0);
    la_write(BMPR13, 0);
    la_write(BMPR14, 0);
    la_write(BMPR15, BMPR15_CLEARALL);

    /* Set non-promiscuous mode (use 0x03 for promiscuous) */
    la_write(DLCR5, (la_read(DLCR5) & ~DLCR5_AM_MASK) | DLCR5_AM_OTHER);

    /* Setup interrupt handler */
    asic_evt_set_handler(ASIC_EVT_EXP_8BIT, la_irq_hnd, NULL);
    asic_evt_enable(ASIC_EVT_EXP_8BIT, ASIC_IRQB);

    /* Enable receive interrupt */
    la_write(DLCR3, DLCR1_PKTRDY);

    /* Enable transmitter / receiver */
    la_write(DLCR6, (la_read(DLCR6) & ~DLCR6_DLCRST));

    la_started = LA_RUNNING;
    return 0;
}

/* Start lan adapter (after a stop) */
static void la_start(void) {
    assert_msg(la_started == LA_PAUSED, "la_start called out of sequence");

    /* Set normal receive mode */
    la_write(DLCR5, (la_read(DLCR5) & ~DLCR5_AM_MASK) | DLCR5_AM_OTHER);
    la_started = LA_RUNNING;
}

/* Temporarily stop lan adapter */
static void la_stop(void) {
    int timeout = 50;

    assert_msg(la_started == LA_RUNNING, "la_stop called out of sequence");

    /* Make sure we aren't transmitting currently */
    while(BMPR10_PKTCNT(la_read(BMPR10)) > 0 && (--timeout) > 0)
        thd_sleep(2);

    /* Disable all receive */
    la_write(DLCR5, (la_read(DLCR5) & ~DLCR5_AM_MASK) | DLCR5_AM_OFF);
    la_started = LA_PAUSED;
}

/* Shut it down for good */
static void la_hw_shutdown(void) {
    /* Power down chip */
    la_write(DLCR7, la_read(DLCR7) & ~DLCR7_NSTBY);

    la_started = LA_NOT_STARTED;

    /* Unhook interrupts */
    asic_evt_disable(ASIC_EVT_EXP_8BIT, ASIC_IRQB);
    asic_evt_remove_handler(ASIC_EVT_EXP_8BIT);
}

/* We don't really need these stats right now but we might want 'em later */
static int total_pkts_rx = 0, total_pkts_tx = 0;

/* Transmit a packet */
/* Note that it's technically possible to queue up more than one packet
   at a time for transmission, but this is the simple way. */
static int la_tx(const uint8 * pkt, int len, int blocking) {
    int i, timeout;

    (void)blocking;

    assert_msg(la_started == LA_RUNNING, "la_tx called out of sequence");

    /* Wait for queue to empty */
    timeout = 50;

    while(BMPR10_PKTCNT(la_read(BMPR10)) > 0 && (--timeout) > 0)
        thd_sleep(2);

    if(timeout == 0) {
        dbglog(DBG_ERROR, "la_tx timed out waiting for previous tx\n");
        return 0;
    }

    /* Is the length less than the minimum? */
    if(len < 0x60)
        len = 0x60;

    /* Poke the length */
    la_write(BMPR8, (len & 0x00ff));
    la_write(BMPR8, (len & 0xff00) >> 8);

    /* Write the packet */
    for(i = 0; i < len; i++)
        la_write(BMPR8, pkt[i]);

    /* Start the transmitter */
    thd_sleep(2);
    la_write(BMPR10, 1 | BMPR10_TX);    /* 1 Packet, Start */

    total_pkts_tx++;

    return 1;
}

static unsigned char current_pkt[1514];

/* Check for received packets */
static int la_rx(void) {
    int i, status, len, count;

    assert_msg(la_started == LA_RUNNING, "la_rx called out of sequence");

    for(count = 0; ; count++) {
        /* Is the buffer empty? */
        if(la_read(DLCR5) & DLCR5_BUFEMP)
            return count;

        /* Get the receive status byte */
        status = la_read(BMPR8);
        (void)la_read(BMPR8);

        /* Get the packet length */
        len = la_read(BMPR8);
        len |= la_read(BMPR8) << 8;

        /* Check for errors */
        if((status & 0xF0) != 0x20) {
            dbglog(DBG_ERROR, "la_rx: receive error occurred (status %02x)\n", status);
            return -1;
        }

        /* Read the packet */
        if(len > 1514) {
            dbglog(DBG_ERROR, "la_rx: big packet received (size %d)\n", len);
            return -2;
        }

        for(i = 0; i < len; i++) {
            current_pkt[i] = la_read(BMPR8);
        }

        /* Submit it for processing */
        net_input(&la_if, current_pkt, len);

        total_pkts_rx++;
    }
}

static void la_irq_hnd(uint32 code, void *data) {
    int intr_rx, intr_tx, hnd = 0;

    (void)code;
    (void)data;

    /* Acknowledge Lan Adapter interrupt(s) */
    intr_tx = la_read(DLCR0);
    la_write(DLCR0, intr_tx);
    intr_rx = la_read(DLCR1);
    la_write(DLCR1, intr_rx);

    /* Handle receive interrupt */
    if(intr_rx & DLCR1_PKTRDY) {
        la_rx();
        hnd = 1;
    }

    if(!hnd) {
        dbglog(DBG_KDEBUG, "la_irq_hnd: spurious interrupt for %02x/%02x\n", intr_tx, intr_rx);
    }
}

/****************************************************************************/
/* Netcore interface */

netif_t la_if;

static void set_ipv6_lladdr(void) {
    /* Set up the IPv6 link-local address. This is done in accordance with
       Section 4/5 of RFC 2464 based on the MAC Address of the adapter. */
    la_if.ip6_lladdr.__s6_addr.__s6_addr8[0]  = 0xFE;
    la_if.ip6_lladdr.__s6_addr.__s6_addr8[1]  = 0x80;
    la_if.ip6_lladdr.__s6_addr.__s6_addr8[8]  = la_if.mac_addr[0] ^ 0x02;
    la_if.ip6_lladdr.__s6_addr.__s6_addr8[9]  = la_if.mac_addr[1];
    la_if.ip6_lladdr.__s6_addr.__s6_addr8[10] = la_if.mac_addr[2];
    la_if.ip6_lladdr.__s6_addr.__s6_addr8[11] = 0xFF;
    la_if.ip6_lladdr.__s6_addr.__s6_addr8[12] = 0xFE;
    la_if.ip6_lladdr.__s6_addr.__s6_addr8[13] = la_if.mac_addr[3];
    la_if.ip6_lladdr.__s6_addr.__s6_addr8[14] = la_if.mac_addr[4];
    la_if.ip6_lladdr.__s6_addr.__s6_addr8[15] = la_if.mac_addr[5];
}

static int la_if_detect(netif_t * self) {
    if(self->flags & NETIF_DETECTED)
        return 0;

    if(la_detect() < 0)
        return -1;

    self->flags |= NETIF_DETECTED;
    return 0;
}

static int la_if_init(netif_t * self) {
    if(self->flags & NETIF_INITIALIZED)
        return 0;

    if(la_hw_init() < 0)
        return -1;

    memcpy(self->mac_addr, la_mac, 6);
    set_ipv6_lladdr();
    self->flags |= NETIF_INITIALIZED;
    return 0;
}

static int la_if_shutdown(netif_t * self) {
    if(!(self->flags & NETIF_INITIALIZED))
        return 0;

    la_hw_shutdown();

    self->flags &= ~(NETIF_DETECTED | NETIF_INITIALIZED | NETIF_RUNNING);
    return 0;
}

static int la_if_start(netif_t * self) {
    if(!(self->flags & NETIF_INITIALIZED))
        return -1;

    /* The Lan adapter seems a bit picky if you try to send packets too soon
       after initialization (or at least mine is). 3 seconds seems to be enough
       of a delay, but 2 seconds certainly is not. Give it 4 seconds, just in
       case its in a bad mood this run. */
    thd_sleep(4000);

    self->flags |= NETIF_RUNNING;

    if(la_started == LA_PAUSED)
        la_start();

    return 0;
}

static int la_if_stop(netif_t * self) {
    if(!(self->flags & NETIF_RUNNING))
        return -1;

    self->flags &= ~NETIF_RUNNING;

    if(la_started == LA_RUNNING)
        la_stop();

    return 0;
}

static int la_if_tx(netif_t * self, const uint8 * data, int len, int blocking) {
    if(!(self->flags & NETIF_RUNNING))
        return NETIF_TX_ERROR;

    if(la_tx(data, len, blocking) != 1)
        return NETIF_TX_ERROR;

    return NETIF_TX_OK;
}

/* We'll auto-commit for now */
static int la_if_tx_commit(netif_t * self) {
    (void)self;
    return 0;
}

static int la_if_rx_poll(netif_t * self) {
    (void)self;
    return la_rx();
}

/* Don't need to hook anything here yet */
static int la_if_set_flags(netif_t * self, uint32 flags_and, uint32 flags_or) {
    self->flags = (self->flags & flags_and) | flags_or;
    return 0;
}

static int la_if_set_mc(netif_t *self, const uint8 *list, int count) {
    int i;

    (void)self;

    /* Reset Data Link Control */
    thd_sleep(2);
    la_write(DLCR6, la_read(DLCR6) | DLCR6_DLCRST);
    thd_sleep(2);

    if(count == 0) {
        /* Clear the multicast address filter */
        for(i = 0; i < 8; i++) {
            la_write(MAR8 + i, 0);
        }
    }
    else {
        int pos;
        uint32 tmp;
        uint8 mar[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

        /* Go through each entry and add the value to the filter */
        for(i = 0, pos = 0; i < count; ++i, pos += 6) {
            tmp = net_crc32le(list + pos, 6) >> 26;
            mar[tmp >> 3] |= 1 << (tmp & 7);
        }

        /* Set the multicast address filter */
        for(i = 0; i < 8; i++) {
            la_write(MAR8 + i, mar[i]);
        }
    }

    /* Enable transmitter / receiver */
    la_write(DLCR6, (la_read(DLCR6) & ~DLCR6_DLCRST));

    return 0;
}

/* Set ISP configuration from the flashrom, as long as we're configured statically */
static void la_set_ispcfg(void) {
    flashrom_ispcfg_t isp;

    if(flashrom_get_ispcfg(&isp) == -1)
        return;

    if(isp.method != FLASHROM_ISP_STATIC)
        return;

    if((isp.valid_fields & FLASHROM_ISP_IP)) {
        memcpy(la_if.ip_addr, isp.ip, 4);
    }

    if((isp.valid_fields & FLASHROM_ISP_NETMASK)) {
        memcpy(la_if.netmask, isp.nm, 4);
    }

    if((isp.valid_fields & FLASHROM_ISP_GATEWAY)) {
        memcpy(la_if.gateway, isp.gw, 4);
    }

    if((isp.valid_fields & FLASHROM_ISP_DNS)) {
        memcpy(la_if.dns, isp.dns[0], 4);
    }

    if((isp.valid_fields & FLASHROM_ISP_BROADCAST)) {
        memcpy(la_if.broadcast, isp.bc, 4);
    } else {
        /* Default to 255.255.255.255 */
        memset(la_if.broadcast, 255, 4);
    }
}

/* Initialize */
int la_init(void) {
    /* Initialize our state */
    la_started = LA_NOT_STARTED;

    /* Setup the netcore structure */
    la_if.name = "la";
    la_if.descr = "Lan Adapter (HIT-0300)";
    la_if.index = 0;
    la_if.dev_id = 0;
    la_if.flags = NETIF_NO_FLAGS;
    la_if.if_detect = la_if_detect;

    /* Short circuit if no lan is detected */
    if(la_if.if_detect(&la_if) < 0) {
        dbglog(DBG_KDEBUG, "lan: no device detected\n");
        return -1;
    }

    memset(la_if.ip_addr, 0, sizeof(la_if.ip_addr));
    memset(la_if.netmask, 0, sizeof(la_if.netmask));
    memset(la_if.gateway, 0, sizeof(la_if.gateway));
    memset(la_if.broadcast, 0, sizeof(la_if.broadcast));
    memset(la_if.dns, 0, sizeof(la_if.dns));
    la_if.mtu = 1500; /* The Ethernet v2 MTU */
    memset(&la_if.ip6_lladdr, 0, sizeof(la_if.ip6_lladdr));
    la_if.ip6_addrs = NULL;
    la_if.ip6_addr_count = 0;
    memset(&la_if.ip6_gateway, 0, sizeof(la_if.ip6_gateway));
    la_if.mtu6 = 0;
    la_if.hop_limit = 0;

    la_if.if_init = la_if_init;
    la_if.if_shutdown = la_if_shutdown;
    la_if.if_start = la_if_start;
    la_if.if_stop = la_if_stop;
    la_if.if_tx = la_if_tx;
    la_if.if_tx_commit = la_if_tx_commit;
    la_if.if_rx_poll = la_if_rx_poll;
    la_if.if_set_flags = la_if_set_flags;
    la_if.if_set_mc = la_if_set_mc;

    /* Attempt to set up our IP address et al from the flashrom */
    la_set_ispcfg();

    /* Append it to the chain */
    return net_reg_device(&la_if);
}

/* Shutdown */
int la_shutdown(void) {
    la_if_shutdown(&la_if);
    return 0;
}
