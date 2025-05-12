/* KallistiOS ##version##

   hardware/sci.c
   Copyright (C) 2025 Ruslan Rostovtsev
*/

#include <dc/sci.h>
#include <dc/math.h>
#include <arch/cache.h>
#include <arch/dmac.h>
#include <arch/timer.h>

#include <kos/dbglog.h>
#include <kos/regfield.h>
#include <kos/thread.h>
#include <stdint.h>
#include <stdlib.h>

#define SCIREG08(x) *((volatile uint8_t *)(x))
#define SCIREG16(x) *((volatile uint16_t *)(x))

/* SCI Registers */
#define SCTDR1_ADDR 0xFFE0000C        /* Physical address of transmit register */
#define SCRDR1_ADDR 0xFFE00014        /* Physical address of receive register */
#define SCSMR1  SCIREG08(0xFFE00000)  /* Serial mode register */
#define SCBRR1  SCIREG08(0xFFE00004)  /* Bit rate register */
#define SCSCR1  SCIREG08(0xFFE00008)  /* Serial control register */
#define SCTDR1  SCIREG08(SCTDR1_ADDR) /* Transmit data register */
#define SCSSR1  SCIREG08(0xFFE00010)  /* Serial status register */
#define SCRDR1  SCIREG08(SCRDR1_ADDR) /* Receive data register */
#define SCSPTR1 SCIREG08(0xFFE00018)  /* Serial port register */

/* SCIF Registers */
#define SCSMR2  SCIREG16(0xFFE80000)  /* Serial mode register */
#define SCBRR2  SCIREG08(0xFFE80004)  /* Bit rate register */
#define SCSCR2  SCIREG16(0xFFE80008)  /* Serial control register */
#define SCFTDR2 SCIREG08(0xFFE8000C)  /* Transmit FIFO data register */
#define SCFSR2  SCIREG16(0xFFE80010)  /* Serial status register */
#define SCFRDR2 SCIREG08(0xFFE80014)  /* Receive FIFO data register */
#define SCFCR2  SCIREG16(0xFFE80018)  /* FIFO control register */
#define SCFDR2  SCIREG16(0xFFE8001C)  /* FIFO data count register */
#define SCSPTR2 SCIREG16(0xFFE80020)  /* Serial port register */
#define SCLSR2  SCIREG16(0xFFE80024)  /* Serial line status register */

/* SCFCR (FIFO Control) Register Bits */
#define SCFCR_MCE    BIT(3)  /* Modem Control Enable */

/* Status register bits */
#define TDRE    BIT(7)  /* Transmit data register empty */
#define RDRF    BIT(6)  /* Receive data register full */
#define ORER    BIT(5)  /* Overrun error */
#define FER     BIT(4)  /* Framing error */
#define PER     BIT(3)  /* Parity error */
#define TEND    BIT(2)  /* Transmit end */

/* Serial control register bits */
#define TIE     BIT(7)  /* Transmit interrupt enable */
#define RIE     BIT(6)  /* Receive interrupt enable */
#define TE      BIT(5)  /* Transmit enable */
#define RE      BIT(4)  /* Receive enable */
#define TEIE    BIT(2)  /* Transmit-End interrupt Enable */
#define CKE1    BIT(1)  /* Clock enable bit 1 for external clock selection */
#define CKE0    BIT(0)  /* Clock enable bit 0 */

/* Common clock settings */
#define CKE_INT_CLK   0     /* Internal clock, SCK pin used as I/O port */
#define CKE_EXT_CLK   CKE1  /* External clock, SCK pin as clock input */

/* Serial port register bits */
#define SPB2DT  BIT(0)  /* Serial port break data */
#define SPB2IO  BIT(1)  /* Serial port break IO */
#define SCKDT   BIT(2)  /* Clock data */
#define SCKIO   BIT(3)  /* Clock IO */
#define CTSDT   BIT(4)  /* CTS data */
#define CTSIO   BIT(5)  /* CTS IO */
#define RTSDT   BIT(6)  /* RTS data */
#define RTSIO   BIT(7)  /* RTS IO */

/* Serial mode register bits */
#define SCSMR_CA      BIT(7)  /* Communication mode: 0=Async, 1=Sync (8-bit) */
#define SCSMR_CHR     BIT(6)  /* Character length: 0=8bit, 1=7bit */
#define SCSMR_PE      BIT(5)  /* Parity enable */
#define SCSMR_PM      BIT(4)  /* Parity mode: 0=even, 1=odd */
#define SCSMR_STOP    BIT(3)  /* Stop bit length: 0=1 stop bit, 1=2 stop bits */
#define SCSMR_CKS1    BIT(1)  /* Clock select bit 1 */
#define SCSMR_CKS0    BIT(0)  /* Clock select bit 0 */

/* Clock settings */
#define PERIPHERAL_CLOCK 50000000  /* 50 MHz peripheral clock */

/* Standby control register */
#define STBCR         *((volatile uint8_t *)(0xFFC00004))
#define STBCR_SCI_STP BIT(0)  /* SCI module standby (1=stopped, 0=active) */

/* GPIO registers */
#define PCTRA    *((volatile uint32_t *)(0xFF80002C))
#define PDTRA    *((volatile uint16_t *)(0xFF800030))

#define SCI_SPI_CS_PIN_BIT    7  /* PA7 */

/* Bit positions in PCTRA */
#define SCI_SPI_CS_PIN_POS    (SCI_SPI_CS_PIN_BIT * 2)

/* Bit masks for PCTRA */
#define SCI_SPI_CS_PIN_MASK   GENMASK(SCI_SPI_CS_PIN_POS + 1, SCI_SPI_CS_PIN_POS)  /* 2 bits */
#define SCI_SPI_CS_PIN_CFG    BIT(SCI_SPI_CS_PIN_POS)  /* Configure as output */

/* Bit masks for PDTRA */
#define SCI_SPI_CS_PDTRA_BIT  BIT(SCI_SPI_CS_PIN_BIT)

/* Timeout */
#define SCI_MAX_WAIT_CYCLES 500000

/* Private global variables */
static bool initialized = false;
static sci_mode_t sci_mode = SCI_MODE_NONE;
static sci_spi_cs_mode_t cs_mode = SCI_SPI_CS_NONE;
static uint8_t transfer_mode = 0;

static uint8_t *spi_dma_buffer = NULL;
static size_t spi_buffer_size = 0;

/* 
 * For DMA transmission, we use BURST mode since it's a one-sided transfer where
 * DMA can monopolize the bus without releasing it until the entire transfer
 * is complete. This provides maximum throughput for transmit operations.
 */
static dma_config_t sci_dma_tx_config = {
    .channel = DMA_CHANNEL_1,
    .request = DMA_REQUEST_SCI_TRANSMIT,
    .unit_size = DMA_UNITSIZE_8BIT,
    .src_mode = DMA_ADDRMODE_INCREMENT,
    .dst_mode = DMA_ADDRMODE_FIXED,
    .transmit_mode = DMA_TRANSMITMODE_BURST,
    .callback = NULL
};

/* 
 * For DMA reception, we use CYCLE_STEAL mode to use the hybrid approach in SPI.
 * While DMA handles data reception, the CPU simultaneously transmits dummy bytes
 * to generate clock signals. This mode ensures the bus is shared properly
 * between DMA and CPU operations.
 */
static dma_config_t sci_dma_rx_config = {
    .channel = DMA_CHANNEL_1,
    .request = DMA_REQUEST_SCI_RECEIVE,
    .unit_size = DMA_UNITSIZE_8BIT,
    .src_mode = DMA_ADDRMODE_FIXED,
    .dst_mode = DMA_ADDRMODE_INCREMENT,
    .transmit_mode = DMA_TRANSMITMODE_CYCLE_STEAL,
    .callback = NULL
};

static __always_inline void clear_sci_errors() {
    SCSSR1 &= ~(ORER | FER | PER);
}

static __always_inline sci_result_t check_sci_errors() {
    uint8_t status = SCSSR1;

    if(status & ORER) {
        clear_sci_errors();
        dbglog(DBG_ERROR, "SCI: Overrun error\n");
        return SCI_ERR_OVERRUN;
    }
    else if(status & FER) {
        clear_sci_errors();
        dbglog(DBG_ERROR, "SCI: Framing error\n");
        return SCI_ERR_FRAMING;
    }
    else if(status & PER) {
        clear_sci_errors();
        dbglog(DBG_ERROR, "SCI: Parity error\n");
        return SCI_ERR_PARITY;
    }

    return SCI_OK;
}

static __always_inline void sci_set_transfer_mode(uint8_t mode) {
    uint8_t reg;

    if(mode == 0) {
        reg = SCSCR1;
        reg &= ~(TE | RE);
        SCSCR1 = reg;
    }
    else if(transfer_mode != mode) {
        reg = SCSCR1;

        if(transfer_mode) {
            reg &= ~(TE | RE);
            SCSCR1 = reg;
            timer_spin_delay_ns(1500);
        }
        reg |= mode;
        SCSCR1 = reg;
    }

    transfer_mode = mode;
}


static int calculate_baud_rate(uint32_t baud_rate, sci_mode_t mode, uint8_t *scsmr, uint8_t *scbrr) {
    uint32_t n, divisor, tmp_div, error, best_error = 0xFFFFFFFF;
    uint8_t best_n = 0, best_brr = 0, brr;
    uint32_t actual_baud, best_actual_baud = 0;

    for(n = 0; n < 4; ++n) {

        /* Calculate N value (BRR) according to the formula.
           Also do not use FPU for division and use special case for n=0,
           because FPU can be disabled by compiler. */
        if(mode == SCI_MODE_UART) {
            if(n == 0) {
                /* Special case for n=0: PCLK/(64*0.5*B) - 1 = PCLK/(32*B) - 1 */
                tmp_div = (PERIPHERAL_CLOCK + (32 * baud_rate / 2)) / (32 * baud_rate);
            }
            else {
                divisor = 1 << (2*n - 1); /* 2^(2n-1) */
                tmp_div = (PERIPHERAL_CLOCK + (64 * divisor * baud_rate / 2)) / (64 * divisor * baud_rate);
            }
        }
        else {
            if(n == 0) {
                /* Special case for n=0: PCLK/(8*0.5*B) - 1 = PCLK/(4*B) - 1 */
                tmp_div = (PERIPHERAL_CLOCK + (4 * baud_rate / 2)) / (4 * baud_rate);
            }
            else {
                divisor = 1 << (2*n - 1); /* 2^(2n-1) */
                tmp_div = (PERIPHERAL_CLOCK + (8 * divisor * baud_rate / 2)) / (8 * divisor * baud_rate);
            }
        }

        if(tmp_div >= 1 && tmp_div <= 256) {
            brr = tmp_div - 1;

            /* Calculate actual baud rate */
            if(mode == SCI_MODE_UART) {
                if(n == 0) {
                    actual_baud = PERIPHERAL_CLOCK / (32 * (brr + 1));
                }
                else {
                    divisor = 1 << (2*n - 1);
                    actual_baud = PERIPHERAL_CLOCK / (64 * divisor * (brr + 1));
                }
            }
            else {
                if(n == 0) {
                    actual_baud = PERIPHERAL_CLOCK / (4 * (brr + 1));
                }
                else {
                    divisor = 1 << (2*n - 1);
                    actual_baud = PERIPHERAL_CLOCK / (8 * divisor * (brr + 1));
                }
            }

            error = (actual_baud > baud_rate)
                        ? (actual_baud - baud_rate)
                        : (baud_rate - actual_baud);

            if(error < best_error) {
                best_error = error;
                best_n = n;
                best_brr = brr;
                best_actual_baud = actual_baud;

                if(error <= (baud_rate / 100))
                    break;
            }
        }
    }

    if(best_error == 0xFFFFFFFF) {
        return -1;
    }

    if(best_actual_baud != baud_rate) {
        dbglog(DBG_DEBUG, "SCI: Actual baud rate: %lu, n: %u, BRR: %u\n",
            best_actual_baud, best_n, best_brr);
    }

    /* Set CKS bits (0-3) for clock divider */
    *scsmr = best_n;

    /* Set BRR (0-255) for baud rate */
    *scbrr = best_brr;

    return 0;
}

sci_result_t sci_init(uint32_t baud_rate, sci_mode_t mode, sci_clock_t clock_src) {
    uint8_t scsmr1 = 0, scbrr1, scscr1, dummy;
    uint32_t timeout_cnt;
    sci_result_t result;

    dbglog(DBG_DEBUG, "SCI: Initializing in %s mode at %lu baud\n", 
           (mode == SCI_MODE_UART) ? "UART" : "SPI", baud_rate);

    if(initialized) {
        sci_shutdown();
    }

    /* Enable SCI module (disable standby) */
    if(STBCR & STBCR_SCI_STP) {
        STBCR &= ~STBCR_SCI_STP;
        /* Delay to allow the module to power up */
        thd_sleep(1);
    }

    /* Disable transmit and receive */
    SCSCR1 = 0;
    SCSPTR1 = 0;

    /* Set CKE bits for clock */
    if(clock_src == SCI_CLK_EXT) {
        /* Set CKE bits for external clock */
        scscr1 = CKE_EXT_CLK;
        SCSCR1 = scscr1;
    }
    else {
        scscr1 = CKE_INT_CLK;
    }

    if(calculate_baud_rate(baud_rate, mode, &scsmr1, &scbrr1) < 0) {
        dbglog(DBG_ERROR, "SCI: Failed to calculate baud rate for %lu\n", baud_rate);
        return SCI_ERR_PARAM;
    }

    if(mode == SCI_MODE_UART) {
        sci_configure_uart(SCI_UART_8N1, &scsmr1);
    }
    else if(mode == SCI_MODE_SPI) {
        /* Use 512 bytes DMA buffer for SPI operations by default,
            because it's sector size of SD cards. */
#ifdef __DREAMCAST__
        /* On Dreamcast, we use GPIO for CS (anyway need soldering all pins),
           because RTS can be used for VS-link cable */
        sci_configure_spi(SCI_SPI_CS_GPIO, 512);
#else
        /* On Naomi, we use SCIF RTS for CS, because no GPIO pins on CN1 connector */
        sci_configure_spi(SCI_SPI_CS_RTS, 512);
#endif
        /* Set CA bit for 8-bit synchronous mode */
        scsmr1 |= SCSMR_CA;
    }
    else {
        dbglog(DBG_ERROR, "SCI: Invalid mode\n");
        return SCI_ERR_PARAM;
    }

    SCSMR1 = scsmr1;
    SCBRR1 = scbrr1;

    /* Allow time for the changes to take effect */
    thd_sleep(1);

    if(mode == SCI_MODE_UART) {
        /* Enable transmit and receive */
        transfer_mode = (TE | RE);
        scscr1 |= transfer_mode;
        SCSCR1 = scscr1;
    }
    else {
        transfer_mode = 0;
    }

    /* Clear any error flags */
    clear_sci_errors();

    /* Flush any pending data */
    if(SCSSR1 & RDRF) {
        dummy = SCRDR1;
        (void)dummy;
    }

    /* Wait for TDRE to be set */
    timeout_cnt = 0;
    do {
        result = check_sci_errors();
        if(result != SCI_OK) {
            return result;
        }

        if(++timeout_cnt > SCI_MAX_WAIT_CYCLES) {
            dbglog(DBG_ERROR, "SCI: Timeout waiting for TDRE\n");
            return SCI_ERR_TIMEOUT;
        }
    } while(!(SCSSR1 & TDRE));

    sci_mode = mode;
    initialized = true;

    return SCI_OK;
}

void sci_configure_uart(sci_uart_config_t config, uint8_t *scsmr1) {
    uint8_t smr;

    if(scsmr1 == NULL) {
        SCSSR1 &= ~transfer_mode;
        smr = SCSMR1;
    }
    else {
        smr = *scsmr1;
    }

    smr |= config;

    if(scsmr1 == NULL) {
        SCSMR1 = smr;
        thd_sleep(1);
        SCSCR1 |= transfer_mode;
    }
    else {
        *scsmr1 = smr;
    }
}

static void sci_shutdown_spi_cs() {
    uint16_t sptr;
    uint16_t fcr;

    if(cs_mode == SCI_SPI_CS_GPIO) {
        PCTRA = PCTRA & ~SCI_SPI_CS_PIN_MASK;
    }
    else if(cs_mode == SCI_SPI_CS_RTS) {
        fcr = SCFCR2;
        fcr &= ~SCFCR_MCE;
        SCFCR2 = fcr;

        sptr = SCSPTR2;
        sptr &= ~(RTSIO | RTSDT);
        SCSPTR2 = sptr;
    }
    cs_mode = SCI_SPI_CS_NONE;
}

void sci_configure_spi(sci_spi_cs_mode_t cs, size_t buffer_size) {
    uint16_t sptr;
    uint16_t fcr;

    sci_shutdown_spi_cs();

    /* Allocate a single aligned buffer for both TX and RX DMA operations */
    if (buffer_size > 0) {
        if(spi_dma_buffer != NULL && spi_buffer_size != buffer_size) {
            free(spi_dma_buffer);
            spi_dma_buffer = NULL;
        }
        
        if(spi_dma_buffer == NULL) {
            spi_dma_buffer = aligned_alloc(32, buffer_size);
            if (spi_dma_buffer == NULL) {
                dbglog(DBG_ERROR, "SCI: Failed to allocate DMA buffer\n");
                return;
            }
            spi_buffer_size = buffer_size;
        }
    }

    if(cs == SCI_SPI_CS_GPIO) {
        /* Configure PA7 as output */
        PCTRA = (PCTRA & ~SCI_SPI_CS_PIN_MASK) | SCI_SPI_CS_PIN_CFG;
        /* Initial CS state is inactive (high) */
        PDTRA |= SCI_SPI_CS_PDTRA_BIT;
    }
    else if(cs == SCI_SPI_CS_RTS) {
        /* Modem control disable */
        fcr = SCFCR2;
        fcr &= ~SCFCR_MCE;
        SCFCR2 = fcr;

        /* Set RTS as output (RTSIO=1) and initial state inactive (high, RTSDT=1) */
        sptr = SCSPTR2;
        sptr |= (RTSIO | RTSDT);
        SCSPTR2 = sptr;
    }

    sci_spi_set_cs(0);
    cs_mode = cs;
}

void sci_shutdown() {

    if(!initialized) {
        return;
    }

    /* Disable TX/RX */
    SCSCR1 = 0;
    transfer_mode = 0;

    /* Put SCI back to standby */
    STBCR |= STBCR_SCI_STP;

    sci_shutdown_spi_cs();

    /* Free the DMA buffer if allocated */
    if(spi_dma_buffer != NULL) {
        free(spi_dma_buffer);
        spi_dma_buffer = NULL;
        spi_buffer_size = 0;
    }

    initialized = false;
    sci_mode = SCI_MODE_NONE;
}

sci_result_t sci_write_byte(uint8_t data) {
    uint32_t timeout_cnt = 0;
    sci_result_t result;

    if(!initialized || sci_mode != SCI_MODE_UART) {
        return SCI_ERR_NOT_INITIALIZED;
    }

    /* Wait until transmit buffer is empty */
    do {
        /* Check for errors */
        result = check_sci_errors();
        if(result != SCI_OK) {
            return result;
        }

        if(++timeout_cnt > SCI_MAX_WAIT_CYCLES) {
            dbglog(DBG_ERROR, "SCI: Timeout waiting for TDRE\n");
            return SCI_ERR_TIMEOUT;
        }
    } while(!(SCSSR1 & TDRE));

    /* Send byte and clear TDRE flag */
    SCTDR1 = data;
    SCSSR1 &= ~TDRE;

    return SCI_OK;
}

sci_result_t sci_read_byte(uint8_t *data) {
    uint32_t timeout_cnt = 0;
    sci_result_t result;

    if(!initialized || sci_mode != SCI_MODE_UART) {
        return SCI_ERR_NOT_INITIALIZED;
    }

    if(data == NULL) {
        return SCI_ERR_PARAM;
    }

    /* Wait until data is available */
    timeout_cnt = 0;
    do {
        /* Check for errors */
        result = check_sci_errors();
        if(result != SCI_OK) {
            return result;
        }

        if(++timeout_cnt > SCI_MAX_WAIT_CYCLES) {
            dbglog(DBG_ERROR, "SCI: Timeout waiting for RDRF\n");
            return SCI_ERR_TIMEOUT;
        }
    } while(!(SCSSR1 & RDRF));

    /* Read the received byte and clear RDRF flag */
    *data = SCRDR1;
    SCSSR1 &= ~RDRF;

    return SCI_OK;
}

sci_result_t sci_write_data(uint8_t *data, size_t len) {
    uint32_t timeout_cnt;
    sci_result_t result;

    if(!initialized || sci_mode != SCI_MODE_UART) {
        return SCI_ERR_NOT_INITIALIZED;
    }

    if(data == NULL || len == 0) {
        return SCI_ERR_PARAM;
    }

    while(len--) {
        /* Wait until transmit buffer is empty */
        timeout_cnt = 0;
        do {
            /* Check for errors */
            result = check_sci_errors();
            if(result != SCI_OK) {
                return result;
            }

            if(++timeout_cnt > SCI_MAX_WAIT_CYCLES) {
                dbglog(DBG_ERROR, "SCI: Timeout waiting for TDRE\n");
                return SCI_ERR_TIMEOUT;
            }
        } while(!(SCSSR1 & TDRE));

        /* Send byte and clear TDRE flag */
        SCTDR1 = *data++;
        SCSSR1 &= ~TDRE;
    }

    return SCI_OK;
}

sci_result_t sci_read_data(uint8_t *data, size_t len) {
    uint32_t timeout_cnt;
    sci_result_t result;

    if(!initialized || sci_mode != SCI_MODE_UART) {
        return SCI_ERR_NOT_INITIALIZED;
    }

    if(data == NULL || len == 0) {
        return SCI_ERR_PARAM;
    }

    while(len--) {
        /* Wait until data is available */
        timeout_cnt = 0;
        do {
            /* Check for errors */
            result = check_sci_errors();
            if(result != SCI_OK) {
                return result;
            }

            if(++timeout_cnt > SCI_MAX_WAIT_CYCLES) {
                dbglog(DBG_ERROR, "SCI: Timeout waiting for RDRF\n");
                return SCI_ERR_TIMEOUT;
            }
        } while(!(SCSSR1 & RDRF));

        /* Read byte and clear RDRF flag */
        *data++ = SCRDR1;
        SCSSR1 &= ~RDRF;
    }

    return SCI_OK;
}

sci_result_t sci_dma_write_data(const uint8_t *data, size_t len, dma_callback_t callback, void *cb_data) {
    if(!initialized) {
        return SCI_ERR_NOT_INITIALIZED;
    }

    if(data == NULL || len == 0) {
        return SCI_ERR_PARAM;
    }

    /* Configure DMA */
    dma_config_t config = sci_dma_tx_config;
    config.callback = callback;

    /* Prepare DMA source and destination */
    dma_addr_t src = dma_map_src(data, len);
    dma_addr_t dst = hw_to_dma_addr(SCTDR1_ADDR);

    /* Start the DMA transfer */
    if(dma_transfer(&config, dst, src, len, cb_data) != 0) {
        dbglog(DBG_ERROR, "SCI: Failed to start DMA transfer\n");
        return SCI_ERR_DMA;
    }

    /* If no callback was provided, wait for completion */
    if(callback == NULL) {
        return sci_dma_wait_complete();
    }

    return check_sci_errors();
}

sci_result_t sci_dma_read_data(uint8_t *data, size_t len, dma_callback_t callback, void *cb_data) {
    if(!initialized) {
        return SCI_ERR_NOT_INITIALIZED;
    }

    if(data == NULL || len == 0) {
        return SCI_ERR_PARAM;
    }

    /* Configure DMA */
    dma_config_t config = sci_dma_rx_config;
    config.callback = callback;

    /* Prepare DMA source and destination */
    dma_addr_t src = hw_to_dma_addr(SCRDR1_ADDR);
    dma_addr_t dst = dma_map_dst(data, len);

    /* Start the DMA transfer */
    if(dma_transfer(&config, dst, src, len, cb_data) != 0) {
        dbglog(DBG_ERROR, "SCI: Failed to start DMA transfer\n");
        return SCI_ERR_DMA;
    }

    /* If no callback was provided, wait for completion */
    if(callback == NULL) {
        return sci_dma_wait_complete();
    }

    return check_sci_errors();
}

void sci_spi_set_cs(bool enabled) {
    uint16_t val;

    if(cs_mode == SCI_SPI_CS_GPIO) {
        val = PDTRA;

        if(enabled) {
            val &= ~SCI_SPI_CS_PDTRA_BIT;  /* Active CS = Low */
        }
        else {
            val |= SCI_SPI_CS_PDTRA_BIT;   /* Inactive CS = High */
        }

        PDTRA = val;
    }
    else if(cs_mode == SCI_SPI_CS_RTS) {
        val = SCSPTR2;

        if(enabled) {   
            val &= ~RTSDT;  /* Active CS = Low */
        }
        else {
            val |= RTSDT;   /* Inactive CS = High */
        }

        SCSPTR2 = val;
    }
}

sci_result_t sci_spi_rw_byte(uint8_t b, uint8_t *data) {
    uint32_t timeout_cnt;
    uint8_t byte, status;

    if(!initialized || sci_mode != SCI_MODE_SPI) {
        return SCI_ERR_NOT_INITIALIZED;
    }

    if(data == NULL) {
        return SCI_ERR_PARAM;
    }
    /* Set for full-duplex mode */
    sci_set_transfer_mode(TE | RE);

    /* Prepare byte to send with correct bit order */
    byte = bit_reverse8(b);

    /* Wait until transmit buffer is empty */
    timeout_cnt = 0;
    while(!(SCSSR1 & TDRE)) {
        if(++timeout_cnt > SCI_MAX_WAIT_CYCLES) {
            sci_set_transfer_mode(0);
            dbglog(DBG_ERROR, "SCI: Timeout waiting for TDRE in SPI rw byte\n");
            return SCI_ERR_TIMEOUT;
        }
    }

    /* Send a byte and clear TDRE flag */
    SCTDR1 = byte;
    SCSSR1 &= ~TDRE;

    do {
        status = SCSSR1;

        if(status & ORER) {
            SCSSR1 &= ~ORER;
            sci_set_transfer_mode(0);
            dbglog(DBG_ERROR, "SCI: Overrun error\n");
            return SCI_ERR_OVERRUN;
        }
    } while(!(status & RDRF));

    /* Get received byte and clear RDRF flag */
    byte = SCRDR1;
    SCSSR1 &= ~RDRF;

    *data = bit_reverse8(byte);

    return SCI_OK;
}

sci_result_t sci_spi_rw_data(const uint8_t *tx_data, uint8_t *rx_data, size_t len) {
    uint32_t timeout_cnt;
    uint8_t byte, status;

    if(!initialized || sci_mode != SCI_MODE_SPI) {
        return SCI_ERR_NOT_INITIALIZED;
    }

    if(tx_data == NULL || rx_data == NULL || len == 0) {
        return SCI_ERR_PARAM;
    }

    /* Set for full-duplex mode */
    sci_set_transfer_mode(TE | RE);

    while(len--) {

        /* Prepare byte to send with correct bit order */
        byte = bit_reverse8(*tx_data++);

        /* Wait for transmit register to be empty */
        timeout_cnt = 0;
        while(!(SCSSR1 & TDRE)) {
            if(++timeout_cnt > SCI_MAX_WAIT_CYCLES) {
                sci_set_transfer_mode(0);
                dbglog(DBG_ERROR, "SCI: Timeout waiting for TDRE in SPI rw\n");
                return SCI_ERR_TIMEOUT;
            }
        }

        /* Send byte */
        SCTDR1 = byte;

        /* Send a byte and clock in the received byte */
        SCSSR1 &= ~TDRE;

        /* Wait for receive register to be full */
        do {
            status = SCSSR1;

            if(status & ORER) {
                SCSSR1 &= ~ORER;
                sci_set_transfer_mode(0);
                dbglog(DBG_ERROR, "SCI: Overrun error\n");
                return SCI_ERR_OVERRUN;
            }
        } while(!(status & RDRF));

        /* Read the received byte and clear RDRF flag */
        byte = SCRDR1;
        SCSSR1 &= ~RDRF;

        *rx_data++ = bit_reverse8(byte);
    }

    return SCI_OK;
}

sci_result_t sci_spi_write_byte(uint8_t b) {
    uint32_t timeout_cnt;
    uint8_t byte;

    if(!initialized || sci_mode != SCI_MODE_SPI) {
        return SCI_ERR_NOT_INITIALIZED;
    }

    /* Set for transmit-only mode */
    sci_set_transfer_mode(TE);

    /* Prepare byte with correct bit order */
    byte = bit_reverse8(b);

    /* Wait until transmit buffer is empty */
    timeout_cnt = 0;
    while(!(SCSSR1 & TDRE)) {
        if(++timeout_cnt > SCI_MAX_WAIT_CYCLES) {
            sci_set_transfer_mode(0);
            dbglog(DBG_ERROR, "SCI: Timeout waiting for TDRE in SPI write\n");
            return SCI_ERR_TIMEOUT;
        }
    }

    /* Send a byte and clear TDRE flag */
    SCTDR1 = byte;
    SCSSR1 &= ~TDRE;

    /* Wait for TEND flag to be set */
    timeout_cnt = 0;
    while(!(SCSSR1 & TEND)) {
        if(++timeout_cnt > SCI_MAX_WAIT_CYCLES) {
            sci_set_transfer_mode(0);
            dbglog(DBG_ERROR, "SCI: Timeout waiting for TEND in SPI write data\n");
            return SCI_ERR_TIMEOUT;
        }
    }

    return SCI_OK;
}

sci_result_t sci_spi_read_byte(uint8_t *data) {
    uint32_t timeout_cnt;
    uint8_t byte, status;

    if(!initialized || sci_mode != SCI_MODE_SPI) {
        return SCI_ERR_NOT_INITIALIZED;
    }

    if(data == NULL) {
        return SCI_ERR_PARAM;
    }

    /* Set for full-duplex mode,
       because SCI cannot clock in received data properly without transmitting. */
    sci_set_transfer_mode(RE | TE);

    /* Wait until transmit buffer is empty */
    timeout_cnt = 0;
    while(!(SCSSR1 & TDRE)) {
        if(++timeout_cnt > SCI_MAX_WAIT_CYCLES) {
            sci_set_transfer_mode(0);
            dbglog(DBG_ERROR, "SCI: Timeout waiting for TDRE in SPI read byte\n");
            return SCI_ERR_TIMEOUT;
        }
    }

    /* Send dummy byte to clock in the received byte */
    SCTDR1 = 0xff;
    SCSSR1 &= ~TDRE;

    /* Wait for receive register to be full */
    do {
        status = SCSSR1;

        if(status & ORER) {
            SCSSR1 &= ~ORER;
            sci_set_transfer_mode(0);
            dbglog(DBG_ERROR, "SCI: Overrun error\n");
            return SCI_ERR_OVERRUN;
        }
    } while(!(status & RDRF));

    /* Read the received byte and clear RDRF flag */
    byte = SCRDR1;
    SCSSR1 &= ~RDRF;

    *data = bit_reverse8(byte);

    return SCI_OK;
}

sci_result_t sci_spi_write_data(const uint8_t *tx_data, size_t len) {
    uint32_t timeout_cnt;
    uint8_t byte;

    if(!initialized || sci_mode != SCI_MODE_SPI) {
        return SCI_ERR_NOT_INITIALIZED;
    }

    if(tx_data == NULL || len == 0) {
        return SCI_ERR_PARAM;
    }

    /* Set for transmit-only mode */
    sci_set_transfer_mode(TE);

    while(len--) {

        /* Prepare byte with correct bit order */
        byte = bit_reverse8(*tx_data++);

        /* Wait for transmit register to be empty */
        timeout_cnt = 0;
        while(!(SCSSR1 & TDRE)) {
            if(++timeout_cnt > SCI_MAX_WAIT_CYCLES) {
                sci_set_transfer_mode(0);
                dbglog(DBG_ERROR, "SCI: Timeout waiting for TDRE in SPI write data\n");
                return SCI_ERR_TIMEOUT;
            }
        }

        /* Send byte and clear TDRE flag */
        SCTDR1 = byte;
        SCSSR1 &= ~TDRE;
    }

    /* Wait for TEND flag to be set */
    timeout_cnt = 0;
    while(!(SCSSR1 & TEND)) {
        if(++timeout_cnt > SCI_MAX_WAIT_CYCLES) {
            sci_set_transfer_mode(0);
            dbglog(DBG_ERROR, "SCI: Timeout waiting for TEND in SPI write data\n");
            return SCI_ERR_TIMEOUT;
        }
    }

    return SCI_OK;
}

sci_result_t sci_spi_read_data(uint8_t *rx_data, size_t len) {
    uint32_t timeout_cnt = 0;
    uint8_t byte, status;

    if(!initialized || sci_mode != SCI_MODE_SPI) {
        return SCI_ERR_NOT_INITIALIZED;
    }

    if(rx_data == NULL || len == 0) {
        return SCI_ERR_PARAM;
    }

    /* Set for full-duplex mode,
       because SCI cannot clock in received data properly without transmitting. */
    sci_set_transfer_mode(RE | TE);

    /* Wait until transmit buffer is empty */
    while(!(SCSSR1 & TDRE)) {
        if(++timeout_cnt > SCI_MAX_WAIT_CYCLES) {
            sci_set_transfer_mode(0);
            dbglog(DBG_ERROR, "SCI: Timeout waiting for TDRE in SPI read data\n");
            return SCI_ERR_TIMEOUT;
        }
    }
    /* Fill transmit register with dummy byte */
    SCTDR1 = 0xff;

    while(len--) {

        /* Send dummy byte to clock in the received byte */
        SCSSR1 &= ~TDRE;

        /* Wait for receive register to be full */
        do {
            status = SCSSR1;

            if(status & ORER) {
                SCSSR1 &= ~ORER;
                sci_set_transfer_mode(0);
                dbglog(DBG_ERROR, "SCI: Overrun error\n");
                return SCI_ERR_OVERRUN;
            }
        } while(!(status & RDRF));

        /* Read the received byte and clear RDRF flag */
        byte = SCRDR1;
        SCSSR1 &= ~RDRF;

        *rx_data++ = bit_reverse8(byte);
    }

    return SCI_OK;
}

sci_result_t sci_spi_dma_write_data(const uint8_t *data, size_t len, dma_callback_t callback, void *cb_data) {
    size_t i;
    sci_result_t result;
    uint32_t timeout_cnt = 0;

    if(!initialized || sci_mode != SCI_MODE_SPI) {
        return SCI_ERR_NOT_INITIALIZED;
    }

    if(data == NULL || len == 0 || spi_dma_buffer == NULL || len > spi_buffer_size) {
        return SCI_ERR_PARAM;
    }

    /* Reverse each byte */
    for(i = 0; i < len; i++) {
        spi_dma_buffer[i] = bit_reverse8(data[i]);
    }

    /* Configure DMA */
    dma_config_t config = sci_dma_tx_config;
    config.callback = callback;

    /* Prepare DMA */
    dma_addr_t src = dma_map_src(spi_dma_buffer, len);
    dma_addr_t dst = hw_to_dma_addr(SCTDR1_ADDR);

    /* Enable transmission */
    sci_set_transfer_mode(TE);

    /* Start DMA */
    if(dma_transfer(&config, dst, src, len, cb_data) != 0) {
        dbglog(DBG_ERROR, "SCI: Failed to start SPI DMA write\n");
        return SCI_ERR_DMA;
    }

    /* If no callback was provided, wait for completion */
    if(callback == NULL) {

        result = sci_dma_wait_complete();
        if (result != SCI_OK) {
            return result;
        }

        while(!(SCSSR1 & TEND)) {
            if(++timeout_cnt > SCI_MAX_WAIT_CYCLES) {
                sci_set_transfer_mode(0);
                dbglog(DBG_ERROR, "SCI: Timeout waiting for TEND in SPI read data\n");
                return SCI_ERR_TIMEOUT;
            }
        }

        return result;
    }

    return check_sci_errors();
}

sci_result_t sci_spi_dma_read_data(uint8_t *data, size_t len, dma_callback_t callback, void *cb_data) {
    size_t i;
    sci_result_t result;
    uint32_t timeout_cnt;
    uint8_t *buffer = spi_dma_buffer;

    if(!initialized || sci_mode != SCI_MODE_SPI) {
        return SCI_ERR_NOT_INITIALIZED;
    }

    if(data == NULL || len == 0 || spi_dma_buffer == NULL || len > spi_buffer_size) {
        return SCI_ERR_PARAM;
    }

    /* Configure DMA */
    dma_config_t config = sci_dma_rx_config;
    config.callback = callback;

    /* Prepare DMA */
    dma_addr_t src = hw_to_dma_addr(SCRDR1_ADDR);
    dma_addr_t dst = dma_map_dst(spi_dma_buffer, len);

    /* Enable full-duplex mode */
    sci_set_transfer_mode(RE | TE);

    /* Start DMA for receiving data */
    if(dma_transfer(&config, dst, src, len, NULL) != 0) {
        dbglog(DBG_ERROR, "SCI: Failed to start SPI DMA read\n");
        return SCI_ERR_DMA;
    }

    /* Clock in the received bytes by transmitting the dummy bytes */
    for(i = 0; i < len; i++) {
        /* Wait for transmit register to be empty */
        timeout_cnt = 0;
        while(!(SCSSR1 & TDRE)) {
            if(++timeout_cnt > SCI_MAX_WAIT_CYCLES) {
                sci_set_transfer_mode(0);
                dbglog(DBG_ERROR, "SCI: Timeout waiting for TDRE in SPI read data\n");
                return SCI_ERR_TIMEOUT;
            }
        }
        /* Send dummy byte to clock in the received byte */
        SCTDR1 = 0xff;
        SCSSR1 &= ~TDRE;

        /* Perform bit reversal while waiting for TDRE flag */
        if(i > 32) {
            *data++ = bit_reverse8(*buffer++);
        }
    }

    timeout_cnt = 0;
    while(!(SCSSR1 & TEND)) {
        if(++timeout_cnt > SCI_MAX_WAIT_CYCLES) {
            sci_set_transfer_mode(0);
            dbglog(DBG_ERROR, "SCI: Timeout waiting for TEND in SPI read data\n");
            return SCI_ERR_TIMEOUT;
        }
    }

    result = sci_dma_wait_complete();

    if(result != SCI_OK) {
        return result;
    }

    /* Perform bit reversal after DMA completes for the last 33 bytes */
    for(i = 0; i < 33; i++) {
        *data++ = bit_reverse8(*buffer++);
    }

    if(callback) {
        callback(cb_data);
    }

    return result;
}

sci_result_t sci_dma_wait_complete(void) {
    dma_wait_complete(DMA_CHANNEL_1);
    return check_sci_errors();
}
