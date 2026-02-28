/* KallistiOS ##version##

   network/w5500_adapter.c

   Copyright (C) 2025 Ruslan Rostovtsev
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdalign.h>

#include <kos/net.h>
#include <kos/thread.h>
#include <kos/dbglog.h>

#include <arch/timer.h>
#include <dc/sci.h>
#include <dc/scif.h>
#include <dc/flashrom.h>
#include <dc/syscalls.h>
#include <kos/mutex.h>

/* W5500 Register Definitions & Constants */
#define W5500_COMMON_BLOCK 0x00
#define W5500_S0_REG_BLOCK 0x01
#define W5500_S0_TX_BLOCK  0x02
#define W5500_S0_RX_BLOCK  0x03

#define W5500_MR        0x0000
#define W5500_SHAR      0x0009
#define W5500_VERSIONR  0x0039
#define W5500_PHYCFGR   0x002E

#define Sn_MR           0x0000
#define Sn_CR           0x0001
#define Sn_IR           0x0002
#define Sn_SR           0x0003
#define Sn_PORT         0x0004
#define Sn_RXBUF_SIZE   0x001E
#define Sn_TXBUF_SIZE   0x001F
#define Sn_TX_FSR       0x0020
#define Sn_TX_RD        0x0022
#define Sn_TX_WR        0x0024
#define Sn_RX_RSR       0x0026
#define Sn_RX_RD        0x0028
#define Sn_RX_WR        0x002A
#define Sn_IMR          0x002C

/* Commands */
#define CR_OPEN         0x01
#define CR_CLOSE        0x10
#define CR_SEND         0x20
#define CR_RECV         0x40

/* Modes */
#define MR_RST          0x80
#define Sn_MR_MACRAW    0x04
#define Sn_MR_MFEN      0x80

/* Socket Status */
#define SOCK_MACRAW     0x42

/* Interrupts */
#define Sn_IR_SENDOK    0x10
#define Sn_IR_RECV      0x04

/* SPI Control Bits */
#define W5500_SPI_READ  (0x00 << 2)
#define W5500_SPI_WRITE (0x01 << 2)
#define W5500_SPI_VDM   0x00

#define MAC_FILTER_SIZE 16

typedef enum {
    W5500_IF_SCI = 0,
    W5500_IF_SCIF = 1
} w5500_interface_t;

static w5500_interface_t current_interface = W5500_IF_SCI;
static kthread_t *w5500_rx_thread = NULL;
static bool w5500_rx_exit = false;
static bool w5500_use_thread = false;
static bool w5500_registered = false;
static mutex_t w5500_spi_mutex = MUTEX_INITIALIZER;

static uint8_t w5500_mc_list[MAC_FILTER_SIZE * 6];
static int w5500_mc_count = 0;

netif_t w5500_if;

/* Function pointers for SPI interface */
static void (*spi_set_cs)(bool enabled) = NULL;
static int (*spi_init)(void) = NULL;
static void (*spi_shutdown)(void) = NULL;
static int (*spi_read_data)(uint8_t *data, size_t len) = NULL;
static int (*spi_write_data)(const uint8_t *data, size_t len) = NULL;
static uint8_t (*spi_read_byte)(void) = NULL;
static void (*spi_write_byte)(uint8_t data) = NULL;

/* Wrappers */
static int scif_read_data_wrapper(uint8_t *data, size_t len) {
    scif_spi_read_data(data, len);
    return 0;
}

static int scif_write_data_wrapper(const uint8_t *data, size_t len) {
    while(len--) {
        scif_spi_write_byte(*data++);
    }
    return 0;
}

static void scif_shutdown_wrapper(void) {
    scif_spi_shutdown();
}

static int scif_init_wrapper(void) {
    return scif_spi_init();
}

static void scif_set_cs_wrapper(bool enabled) {
    if (enabled) mutex_lock(&w5500_spi_mutex);
    scif_spi_set_cs(enabled ? 0 : 1);
    if (!enabled) mutex_unlock(&w5500_spi_mutex);
}

static uint8_t sci_read_byte_wrapper(void) {
    uint8_t rx;
    sci_spi_read_byte(&rx);
    return rx;
}

static void sci_write_byte_wrapper(uint8_t data) {
    sci_spi_write_byte(data);
}

static int sci_read_data_wrapper(uint8_t *data, size_t len) {
    if(len >= 128) {

        size_t read_len = len & ~31;
        int result = sci_spi_dma_read_data(data, read_len, NULL, NULL);

        if(result != SCI_OK) {
            return result;
        }
        if(read_len == len) {
            return 0;
        }
        data += read_len;
        len -= read_len;
    }
    return sci_spi_read_data(data, len);
}

static int sci_write_data_wrapper(const uint8_t *data, size_t len) {
    if(len >= 128) {

        size_t write_len = len & ~31;
        int result = sci_spi_dma_write_data(data, write_len, NULL, NULL);

        if(result != SCI_OK) {
            return result;
        }
        if(write_len == len) {
            return 0;
        }
        data += write_len;
        len -= write_len;
    }
    return sci_spi_write_data(data, len);
}

static void sci_shutdown_wrapper(void) {
    sci_shutdown();
}

static int sci_init_wrapper(void) {
    return sci_init(SCI_SPI_BAUD_MAX, SCI_MODE_SPI, SCI_CLK_INT, 1600);
}

static void sci_set_cs_wrapper(bool enabled) {
    if (enabled) mutex_lock(&w5500_spi_mutex);
    sci_spi_set_cs(enabled);
    if (!enabled) mutex_unlock(&w5500_spi_mutex);
}

static int w5500_spi_init(void) {
    if (current_interface == W5500_IF_SCIF) {
        spi_set_cs = &scif_set_cs_wrapper;
        spi_init = &scif_init_wrapper;
        spi_shutdown = &scif_shutdown_wrapper;
        spi_read_data = &scif_read_data_wrapper;
        spi_write_data = &scif_write_data_wrapper;
        spi_read_byte = &scif_spi_read_byte;
        spi_write_byte = &scif_spi_write_byte;
    }
    else {
        spi_set_cs = &sci_set_cs_wrapper;
        spi_init = &sci_init_wrapper;
        spi_shutdown = &sci_shutdown_wrapper;
        spi_read_data = &sci_read_data_wrapper;
        spi_write_data = &sci_write_data_wrapper;
        spi_read_byte = &sci_read_byte_wrapper;
        spi_write_byte = &sci_write_byte_wrapper;
    }
    return spi_init();
}

/* W5500 I/O Functions */
static uint8_t w5500_read_reg(uint8_t block, uint16_t addr) {
    uint8_t ret;
    uint8_t cmd[3];

    cmd[0] = (addr & 0xFF00) >> 8;
    cmd[1] = addr & 0x00FF;
    cmd[2] = (block << 3) | W5500_SPI_READ | W5500_SPI_VDM;

    spi_set_cs(true);
    spi_write_data(cmd, 3);
    ret = spi_read_byte();
    spi_set_cs(false);

    return ret;
}

static void w5500_write_reg(uint8_t block, uint16_t addr, uint8_t data) {
    uint8_t cmd[4];

    cmd[0] = (addr & 0xFF00) >> 8;
    cmd[1] = addr & 0x00FF;
    cmd[2] = (block << 3) | W5500_SPI_WRITE | W5500_SPI_VDM;
    cmd[3] = data;

    spi_set_cs(true);
    spi_write_data(cmd, 4);
    spi_set_cs(false);
}

static uint16_t w5500_read_reg16(uint8_t block, uint16_t addr) {
    uint16_t ret;
    uint8_t cmd[3];
    uint8_t data[2];

    cmd[0] = (addr & 0xFF00) >> 8;
    cmd[1] = addr & 0x00FF;
    cmd[2] = (block << 3) | W5500_SPI_READ | W5500_SPI_VDM;

    spi_set_cs(true);
    spi_write_data(cmd, 3);
    spi_read_data(data, 2);
    spi_set_cs(false);

    ret = (data[0] << 8) | data[1];
    return ret;
}

static void w5500_write_reg16(uint8_t block, uint16_t addr, uint16_t data) {
    uint8_t cmd[5];

    cmd[0] = (addr & 0xFF00) >> 8;
    cmd[1] = addr & 0x00FF;
    cmd[2] = (block << 3) | W5500_SPI_WRITE | W5500_SPI_VDM;
    cmd[3] = (data & 0xFF00) >> 8;
    cmd[4] = data & 0x00FF;

    spi_set_cs(true);
    spi_write_data(cmd, 5);
    spi_set_cs(false);
}

static void w5500_read_buf(uint8_t block, uint16_t addr, uint8_t *buf, uint16_t len) {
    uint8_t cmd[3];

    cmd[0] = (addr & 0xFF00) >> 8;
    cmd[1] = addr & 0x00FF;
    cmd[2] = (block << 3) | W5500_SPI_READ | W5500_SPI_VDM;

    spi_set_cs(true);
    spi_write_data(cmd, 3);
    spi_read_data(buf, len);
    spi_set_cs(false);
}

static void w5500_write_buf(uint8_t block, uint16_t addr, uint8_t *buf, uint16_t len) {
    uint8_t cmd[3];

    cmd[0] = (addr & 0xFF00) >> 8;
    cmd[1] = addr & 0x00FF;
    cmd[2] = (block << 3) | W5500_SPI_WRITE | W5500_SPI_VDM;

    spi_set_cs(true);
    spi_write_data(cmd, 3);
    spi_write_data(buf, len);
    spi_set_cs(false);
}

static uint16_t w5500_read_reg16_safe(uint8_t block, uint16_t addr) {
    uint16_t val = 0, val1 = 0;

    do {
        val1 = w5500_read_reg16(block, addr);
        if(val1 != 0) {
            val = w5500_read_reg16(block, addr);
        }
    } while(val != val1);
    return val;
}

static int w5500_exec_cmd(uint8_t block, uint8_t cmd) {
    int i = 0;
    uint8_t cmd_buf[4];
    uint8_t status;

    cmd_buf[0] = (Sn_CR & 0xFF00) >> 8;
    cmd_buf[1] = Sn_CR & 0x00FF;
    cmd_buf[2] = (block << 3) | W5500_SPI_WRITE | W5500_SPI_VDM;
    cmd_buf[3] = cmd;

    spi_set_cs(true);
    spi_write_data(cmd_buf, 4);

    cmd_buf[0] = (Sn_CR & 0xFF00) >> 8;
    cmd_buf[1] = Sn_CR & 0x00FF;
    cmd_buf[2] = (block << 3) | W5500_SPI_READ | W5500_SPI_VDM;

    while(1) {
        spi_write_data(cmd_buf, 3);
        status = spi_read_byte();

        if(status == 0) break;

        if(i++ > 1000) {
            spi_set_cs(false);
            return -1;
        }
        thd_pass();
    }

    spi_set_cs(false);
    return 0;
}

static int w5500_wait_link(bool check_hw) {
    int i = 0;
    uint8_t phy_cfg;
    static bool link_status = false;

    do {
        phy_cfg = check_hw ?
            w5500_read_reg(W5500_COMMON_BLOCK, W5500_PHYCFGR) : link_status;

        if(phy_cfg & 1) {
            if(!link_status) {
                link_status = true;
                dbglog(DBG_INFO, "w5500: Link up\n");
            }
            return 0;
        }
        else if(link_status) {
            link_status = false;
            dbglog(DBG_INFO, "w5500: Link down\n");
        }
        thd_sleep(50);
    } while(i++ < 200);

    return -1;
}

static int w5500_soft_reset(void) {
    int i = 0;

    /* Issue Soft Reset Command */
    w5500_write_reg(W5500_COMMON_BLOCK, W5500_MR, MR_RST);

    /* Wait for reset to complete */
    while (w5500_read_reg(W5500_COMMON_BLOCK, W5500_MR) & MR_RST) {
        if (i++ > 10) break;
        thd_sleep(1);
    }
    return i > 10 ? -1 : 0;
}

/* Initialization */
static int w5500_probe_interface(w5500_interface_t intf) {
    uint8_t ver;

    current_interface = intf;

    /* Initialize SPI */
    if(w5500_spi_init()) {
        return -1;
    }

    if(w5500_soft_reset()) {
        spi_shutdown();
        return -1;
    }

    /* Check Version */
    ver = w5500_read_reg(W5500_COMMON_BLOCK, W5500_VERSIONR);
    if(ver != 0x04) {
        spi_shutdown();
        return -1;
    }

    spi_shutdown();
    return 0;
}

static int w5500_hw_init(void) {
    int i = 0;
    uint8_t ver;

    /* Initialize SPI */
    if(w5500_spi_init()) {
        return -1;
    }

    if(w5500_soft_reset()) {
        spi_shutdown();
        return -1;
    }

    /* PHY Reset */
    w5500_write_reg(W5500_COMMON_BLOCK, W5500_PHYCFGR, 0x00);

    /* Default Config (Auto-neg, Normal) */
    w5500_write_reg(W5500_COMMON_BLOCK, W5500_PHYCFGR, 0xB8);

    /* Check Version */
    ver = w5500_read_reg(W5500_COMMON_BLOCK, W5500_VERSIONR);
    if(ver != 0x04) {
        dbglog(DBG_ERROR, "w5500: Chip version mismatch (read %02x, expected 0x04)\n", ver);
        spi_shutdown();
        return -1;
    }

    /* Disable all interrupts */
    w5500_write_reg(W5500_COMMON_BLOCK, Sn_IMR, 0x00);

    /* Clear all sockets */
    for(i = 0; i < 8; i++) {
         uint8_t block = (1 + 4 * i);
         w5500_write_reg(block, Sn_RXBUF_SIZE, 0);
         w5500_write_reg(block, Sn_TXBUF_SIZE, 0);
    }

    /* Configure Buffers - Assign 16KB for RX and TX to Socket 0 */
    w5500_write_reg(W5500_S0_REG_BLOCK, Sn_RXBUF_SIZE, 16);
    w5500_write_reg(W5500_S0_REG_BLOCK, Sn_TXBUF_SIZE, 16);

    /* Set MAC Address */
    w5500_write_buf(W5500_COMMON_BLOCK, W5500_SHAR, w5500_if.mac_addr, 6);
    dbglog(DBG_KDEBUG, "w5500: MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
           w5500_if.mac_addr[0], w5500_if.mac_addr[1], w5500_if.mac_addr[2],
           w5500_if.mac_addr[3], w5500_if.mac_addr[4], w5500_if.mac_addr[5]);

    /* Open Socket 0 in MACRAW mode, enable MAC filter */
    w5500_write_reg(W5500_S0_REG_BLOCK, Sn_MR, Sn_MR_MACRAW | Sn_MR_MFEN);

    if(w5500_exec_cmd(W5500_S0_REG_BLOCK, CR_OPEN) < 0) {
        dbglog(DBG_ERROR, "w5500: Timeout waiting for socket open\n");
        spi_shutdown();
        return -1;
    }

    if(w5500_read_reg(W5500_S0_REG_BLOCK, Sn_SR) != SOCK_MACRAW) {
        dbglog(DBG_ERROR, "w5500: Failed to open socket 0 in MACRAW mode\n");
        spi_shutdown();
        return -1;
    }

    return 0;
}

/* Transmission */
static int w5500_tx(const uint8_t *pkt, int len, int blocking) {
    uint16_t fsr, wr_ptr;

    (void)blocking;

    /* Check PHY Link */
    if(w5500_wait_link(false) != 0) {
        return -1;
    }

    /* Check Free Size */
    fsr = w5500_read_reg16_safe(W5500_S0_REG_BLOCK, Sn_TX_FSR);
    if(fsr < len) {
        dbglog(DBG_ERROR, "w5500: TX buffer full\n");
        return -1; 
    }

    /* Write Data */
    wr_ptr = w5500_read_reg16(W5500_S0_REG_BLOCK, Sn_TX_WR);
    w5500_write_buf(W5500_S0_TX_BLOCK, wr_ptr, (uint8_t *)pkt, len);

    /* Update Write Pointer */
    wr_ptr += len;
    w5500_write_reg16(W5500_S0_REG_BLOCK, Sn_TX_WR, wr_ptr);

    /* Issue Send Command */
    if(w5500_exec_cmd(W5500_S0_REG_BLOCK, CR_SEND) < 0) {
        return -1;
    }

    return 0;
}

static int w5500_rx_poll(netif_t *self) {
    uint16_t rsr, rd_ptr, data_len;
    uint8_t head[2];
    int i, work = 0;
    uint16_t read_len;
    uint8_t alignas(32) rx_pkt_buf[1600];

    if(!(self->flags & NETIF_RUNNING))
        return 0;

    /* Check Received Size */
    rsr = w5500_read_reg16_safe(W5500_S0_REG_BLOCK, Sn_RX_RSR);

    if(rsr > 0) {
        work = 1;
        rd_ptr = w5500_read_reg16(W5500_S0_REG_BLOCK, Sn_RX_RD);

        /* Read 2-byte header (packet length) */
        w5500_read_buf(W5500_S0_RX_BLOCK, rd_ptr, head, 2);
        rd_ptr += 2;

        data_len = (head[0] << 8) | head[1];

        if(data_len < 2) {
            /* Invalid size, skip */
            rd_ptr += data_len;
            w5500_write_reg16(W5500_S0_REG_BLOCK, Sn_RX_RD, rd_ptr);
            w5500_exec_cmd(W5500_S0_REG_BLOCK, CR_RECV);
            return work;
        }
        data_len -= 2; // Actual data length

        /* Peek at destination MAC to filter unwanted packets */
        uint8_t dst_mac[6] = {0};
        uint16_t peek_len = (data_len < 6) ? data_len : 6;
        w5500_read_buf(W5500_S0_RX_BLOCK, rd_ptr, dst_mac, peek_len);

        int drop = 0;
        if(!(self->flags & NETIF_PROMISC)) {
            if((dst_mac[0] & 0x01) && (dst_mac[0] != 0xFF)) {
                drop = 1;
                for(i = 0; i < w5500_mc_count; i++) {
                    if(memcmp(dst_mac, w5500_mc_list + (i * 6), 6) == 0) {
                        drop = 0;
                        break;
                    }
                }
            }
        }

        if(drop) {
            rd_ptr += data_len;
            w5500_write_reg16(W5500_S0_REG_BLOCK, Sn_RX_RD, rd_ptr);
            w5500_exec_cmd(W5500_S0_REG_BLOCK, CR_RECV);
            return work;
        }

        /* Read Packet */
        read_len = data_len;

        if(read_len > sizeof(rx_pkt_buf)) {
            read_len = sizeof(rx_pkt_buf);
        }

        w5500_read_buf(W5500_S0_RX_BLOCK, rd_ptr, rx_pkt_buf, read_len);

        /* Advance pointer by full packet size (header + payload) */
        rd_ptr += data_len; // +2 was already added
        w5500_write_reg16(W5500_S0_REG_BLOCK, Sn_RX_RD, rd_ptr);
        w5500_exec_cmd(W5500_S0_REG_BLOCK, CR_RECV);

        net_input(self, rx_pkt_buf, read_len);
    }
    else {
        w5500_wait_link(true);
    }

    return work;
}

/* RX Thread. Unfortunately, we need to use a polling mechanism
   because we don't have a hardware interrupt input from the W5500.
   NOTE: This timing is very important for the performance of the network stack. */
static void *w5500_rx_func(void *param) {
    netif_t *self = (netif_t *)param;
    while(!w5500_rx_exit) {
        if(w5500_rx_poll(self) == 0)
            thd_sleep(7);
        else
            thd_pass();
    }
    return NULL;
}

/* Netif Callbacks */
static int w5500_if_detect(netif_t *self) {
    if(self->flags & NETIF_DETECTED)
        return 0;

    if(w5500_probe_interface(W5500_IF_SCIF) != 0 &&
        w5500_probe_interface(W5500_IF_SCI) != 0) {
        return -1;
    }

    self->flags |= NETIF_DETECTED;
    return 0;
}

static int w5500_if_init(netif_t *self) {
    if(self->flags & NETIF_INITIALIZED)
        return 0;

    if(w5500_hw_init() < 0)
        return -1;

    self->flags |= NETIF_INITIALIZED;
    return 0;
}

static int w5500_if_start(netif_t *self) {
    if(!(self->flags & NETIF_INITIALIZED))
        return -1;
        
    if(self->flags & NETIF_RUNNING)
        return 0;

    /* Check PHY Link */
    if(w5500_wait_link(true) != 0) {
        return -1;
    }

    if(w5500_use_thread) {
        w5500_rx_exit = false;
        w5500_rx_thread = thd_create(0, w5500_rx_func, (void *)self);
        thd_set_label(w5500_rx_thread, "w5500-rx");
    }

    self->flags |= NETIF_RUNNING;
    return 0;
}

static int w5500_if_stop(netif_t *self) {
    if(!(self->flags & NETIF_RUNNING))
        return 0;

    if(w5500_use_thread) {
        w5500_rx_exit = true;
        thd_join(w5500_rx_thread, NULL);
        w5500_rx_thread = NULL;
    }

    self->flags &= ~NETIF_RUNNING;
    return 0;
}

static int w5500_if_shutdown(netif_t *self) {
    if(self->flags & NETIF_RUNNING)
        w5500_if_stop(self);

    w5500_write_reg(W5500_COMMON_BLOCK, W5500_MR, MR_RST);
    spi_shutdown();

    self->flags &= ~NETIF_INITIALIZED;
    return 0;
}

static int w5500_if_tx(netif_t *self, const uint8_t *data, int len, int blocking) {
    if(!(self->flags & NETIF_RUNNING))
        return NETIF_TX_ERROR;

    if(w5500_tx(data, len, blocking) < 0)
        return NETIF_TX_ERROR;

    return NETIF_TX_OK;
}

static void w5500_update_mac_filter(netif_t *self) {
    uint8_t mode;

    /* Read current mode */
    mode = w5500_read_reg(W5500_S0_REG_BLOCK, Sn_MR);

    if(self->flags & NETIF_PROMISC) {
        /* Disable MAC Filter (Promiscuous - receive all) */
        mode &= ~Sn_MR_MFEN;
    }
    else {
        /* Enable MAC Filter */
        mode |= Sn_MR_MFEN;
    }

    /* We need to re-open the socket for the mode change to take effect */
    w5500_exec_cmd(W5500_S0_REG_BLOCK, CR_CLOSE);
    w5500_write_reg(W5500_S0_REG_BLOCK, Sn_MR, mode);
    w5500_exec_cmd(W5500_S0_REG_BLOCK, CR_OPEN);
}

static int w5500_if_set_mc(netif_t *self, const uint8_t *list, int count) {
    if(count > MAC_FILTER_SIZE) {
        count = MAC_FILTER_SIZE;
    }
    w5500_mc_count = count;
    memcpy(w5500_mc_list, list, count * 6);
    w5500_update_mac_filter(self);

    return 0;
}

static int w5500_if_set_flags(netif_t *self, uint32_t flags_and, uint32_t flags_or) {
    uint32_t old_flags = self->flags;

    self->flags = (self->flags & flags_and) | flags_or;

    if((self->flags & NETIF_PROMISC) != (old_flags & NETIF_PROMISC)) {
        w5500_update_mac_filter(self);
    }

    return 0;
}

/* ISP Config Helper */
static void w5500_set_ispcfg(void) {
    flashrom_ispcfg_t isp;

    if(flashrom_get_ispcfg(&isp) == -1)
        return;

    if(isp.method != FLASHROM_ISP_STATIC)
        return;

    if(isp.valid_fields & FLASHROM_ISP_IP)
        memcpy(w5500_if.ip_addr, isp.ip, 4);

    if(isp.valid_fields & FLASHROM_ISP_NETMASK)
        memcpy(w5500_if.netmask, isp.nm, 4);

    if(isp.valid_fields & FLASHROM_ISP_GATEWAY)
        memcpy(w5500_if.gateway, isp.gw, 4);

    if(isp.valid_fields & FLASHROM_ISP_DNS)
        memcpy(w5500_if.dns, isp.dns[0], 4);

    if(isp.valid_fields & FLASHROM_ISP_BROADCAST)
        memcpy(w5500_if.broadcast, isp.bc, 4);
    else
        memset(w5500_if.broadcast, 255, 4);
}

static void w5500_set_ipv6_lladdr(void) {
    /* Set up the IPv6 link-local address. This is done in accordance with
       Section 4/5 of RFC 2464 based on the MAC Address of the adapter. */
    w5500_if.ip6_lladdr.__s6_addr.__s6_addr8[0]  = 0xFE;
    w5500_if.ip6_lladdr.__s6_addr.__s6_addr8[1]  = 0x80;
    w5500_if.ip6_lladdr.__s6_addr.__s6_addr8[8]  = w5500_if.mac_addr[0] ^ 0x02;
    w5500_if.ip6_lladdr.__s6_addr.__s6_addr8[9]  = w5500_if.mac_addr[1];
    w5500_if.ip6_lladdr.__s6_addr.__s6_addr8[10] = w5500_if.mac_addr[2];
    w5500_if.ip6_lladdr.__s6_addr.__s6_addr8[11] = 0xFF;
    w5500_if.ip6_lladdr.__s6_addr.__s6_addr8[12] = 0xFE;
    w5500_if.ip6_lladdr.__s6_addr.__s6_addr8[13] = w5500_if.mac_addr[3];
    w5500_if.ip6_lladdr.__s6_addr.__s6_addr8[14] = w5500_if.mac_addr[4];
    w5500_if.ip6_lladdr.__s6_addr.__s6_addr8[15] = w5500_if.mac_addr[5];
}

/* Driver Initialization */
int w5500_adapter_init(const uint8_t *mac_addr, bool use_thread) {
    uint64_t id;
    uint8_t *id_bytes;

    if(w5500_registered) {
        return 0;
    }

    w5500_use_thread = use_thread;

    w5500_if.name = "w5500";
    w5500_if.descr = "WIZnet W5500 Adapter";
    w5500_if.index = 0;
    w5500_if.dev_id = 0;
    w5500_if.flags = w5500_use_thread ? NETIF_NO_FLAGS : NETIF_NEEDSPOLL;
    w5500_if.if_detect = w5500_if_detect;

    /* Short circuit if no w5500 is detected */
    if(w5500_if.if_detect(&w5500_if) < 0) {
        dbglog(DBG_KDEBUG, "w5500: No device detected\n");
        return -1;
    }

    w5500_if.if_init = w5500_if_init;
    w5500_if.if_shutdown = w5500_if_shutdown;
    w5500_if.if_start = w5500_if_start;
    w5500_if.if_stop = w5500_if_stop;
    w5500_if.if_tx = w5500_if_tx;
    w5500_if.if_tx_commit = NULL; // Auto commit
    w5500_if.if_rx_poll = w5500_rx_poll;
    w5500_if.if_set_flags = w5500_if_set_flags;
    w5500_if.if_set_mc = w5500_if_set_mc;

    if(mac_addr != NULL) {
        memcpy(w5500_if.mac_addr, mac_addr, 6);
    }
    else {
        /* W5500 doesn't have MAC address, so we try to use hardware ID
           to generate a unique MAC Address or use hardcoded one. */
        id = syscall_sysinfo_id();
        id_bytes = (uint8_t *)&id;

        /* Locally Administered */
        w5500_if.mac_addr[0] = 0x02;

        if(id == 0 || id == (uint64_t)-1) {
            w5500_if.mac_addr[1] = 0x09;
            w5500_if.mac_addr[2] = 0xbf;
            w5500_if.mac_addr[3] = 0x72;
            w5500_if.mac_addr[4] = 0x24;
            w5500_if.mac_addr[5] = 0x01;
        }
        else {
            w5500_if.mac_addr[1] = id_bytes[3];
            w5500_if.mac_addr[2] = id_bytes[4];
            w5500_if.mac_addr[3] = id_bytes[5];
            w5500_if.mac_addr[4] = id_bytes[6];
            w5500_if.mac_addr[5] = id_bytes[7];
        }
    }

    memset(w5500_if.ip_addr, 0, sizeof(w5500_if.ip_addr));
    memset(w5500_if.netmask, 0, sizeof(w5500_if.netmask));
    memset(w5500_if.gateway, 0, sizeof(w5500_if.gateway));
    memset(w5500_if.broadcast, 0, sizeof(w5500_if.broadcast));
    memset(w5500_if.dns, 0, sizeof(w5500_if.dns));

    memset(&w5500_if.ip6_lladdr, 0, sizeof(w5500_if.ip6_lladdr));
    w5500_set_ipv6_lladdr();

    w5500_if.ip6_addrs = NULL;
    w5500_if.ip6_addr_count = 0;
    memset(&w5500_if.ip6_gateway, 0, sizeof(w5500_if.ip6_gateway));
    w5500_if.mtu6 = 0;
    w5500_if.hop_limit = 0;
    w5500_if.mtu = 1500;

    w5500_set_ispcfg();

    if(net_reg_device(&w5500_if) == 0) {
        w5500_registered = true;
        return 0;
    }
    return -1;
}

int w5500_adapter_shutdown(void) {
    if(w5500_registered) {
        w5500_if_shutdown(&w5500_if);
        w5500_registered = false;
    }
    return 0;
}
