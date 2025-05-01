/* KallistiOS ##version##

   hardware/sci.c
   Copyright (C) 2025 Ruslan Rostovtsev
*/

#include <dc/sci.h>
#include <arch/timer.h>
#include <arch/types.h>
#include <kos/dbglog.h>
#include <stdint.h>

#define SCIREG08(x) *((volatile uint8_t *)(x))
#define SCIREG16(x) *((volatile uint16_t *)(x))

/* SCI Registers */
#define SCSMR1  SCIREG08(0xFFE00000)  /* Serial mode register */
#define SCBRR1  SCIREG08(0xFFE00004)  /* Bit rate register */
#define SCSCR1  SCIREG08(0xFFE00008)  /* Serial control register */
#define SCTDR1  SCIREG08(0xFFE0000C)  /* Transmit data register */
#define SCSSR1  SCIREG08(0xFFE00010)  /* Serial status register */
#define SCRDR1  SCIREG08(0xFFE00014)  /* Receive data register */
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
#define SCFCR_MCE    0x08             /* Modem Control Enable */

/* Status register bits */
#define TDRE    0x80  /* Transmit data register empty */
#define RDRF    0x40  /* Receive data register full */
#define ORER    0x20  /* Overrun error */
#define FER     0x10  /* Framing error */
#define PER     0x08  /* Parity error */
#define TEND    0x04  /* Transmit end */

/* Serial control register bits */
#define TE      0x20  /* Transmit enable */
#define RE      0x10  /* Receive enable */

/* Serial port register bits */
#define SPB2DT  0x01  /* Serial port break data */
#define SPB2IO  0x02  /* Serial port break IO */
#define SCKDT   0x04  /* Clock data */
#define SCKIO   0x08  /* Clock IO */
#define CTSDT   0x10  /* CTS data */
#define CTSIO   0x20  /* CTS IO */
#define RTSDT   0x40  /* RTS data */
#define RTSIO   0x80  /* RTS IO */

/* Clock settings */
#define PERIPHERAL_CLOCK 50000000  /* 50 MHz peripheral clock */

/* Standby control register */
#define STBCR         *((volatile uint8_t *)(0xFFC00004))
#define STBCR_SCI_STP 0x01  /* SCI module standby (1=stopped, 0=active) */

/* GPIO registers */
#define PCTRA    *((volatile uint32_t *)(0xFF80002C))
#define PDTRA    *((volatile uint16_t *)(0xFF800030))

#define SCI_SPI_CS_PIN_BIT    7  /* PA7 */

/* Bit positions in PCTRA */
#define SCI_SPI_CS_PIN_POS    (SCI_SPI_CS_PIN_BIT * 2)

/* Bit masks for PCTRA */
#define SCI_SPI_CS_PIN_MASK   (0x03 << SCI_SPI_CS_PIN_POS)
#define SCI_SPI_CS_PIN_CFG    (0x01 << SCI_SPI_CS_PIN_POS)  /* Configure as output */

/* Bit masks for PDTRA */
#define SCI_SPI_CS_PDTRA_BIT  (1 << SCI_SPI_CS_PIN_BIT)

/* Timeout */
#define SCI_MAX_WAIT_CYCLES 500000

/* Private global variables */
static bool initialized = false;
static sci_mode_t sci_mode = SCI_MODE_NONE;
static sci_spi_cs_mode_t cs_mode = SCI_SPI_CS_NONE;
static uint8_t transfer_mode = 0;

uint8_t reverse_bits(uint8_t b);

static inline void clear_sci_errors() {
    SCSSR1 &= ~(ORER | FER | PER);
}

static inline sci_result_t check_sci_errors() {
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

static inline void spi_delay(int count) {
    int i;
    for(i = 0; i < count; i++) {
        __asm__("nop");
    }
}

static inline void sci_set_transfer_mode(uint8_t mode) {
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
            spi_delay(50);
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
        spi_delay(10000);
    }

    /* Disable transmit and receive */
    SCSCR1 = 0;
    SCSPTR1 = 0;

    /* Set CKE bits for clock */
    if(clock_src == SCI_CLK_EXT) {
        /* Set CKE bits for external clock */
        scscr1 = 0x02;
        SCSCR1 = scscr1;
    }
    else {
        scscr1 = 0;
    }

    if(calculate_baud_rate(baud_rate, mode, &scsmr1, &scbrr1) < 0) {
        dbglog(DBG_ERROR, "SCI: Failed to calculate baud rate for %lu\n", baud_rate);
        return SCI_ERR_PARAM;
    }

    if(mode == SCI_MODE_UART) {
        sci_configure_uart(SCI_UART_8N1, &scsmr1);
    }
    else if(mode == SCI_MODE_SPI) {
#ifdef __DREAMCAST__
        /* On Dreamcast, we use GPIO for CS (anyway need soldering all pins),
           because RTS can be used for VS-link cable */
        sci_configure_spi(SCI_SPI_CS_GPIO);
#else
        /* On Naomi, we use SCIF RTS for CS, because no GPIO pins on CN1 connector */
        sci_configure_spi(SCI_SPI_CS_RTS);
#endif
        /* Set CA bit for 8-bit synchronous mode */
        scsmr1 |= 0x80;
    }
    else {
        dbglog(DBG_ERROR, "SCI: Invalid mode\n");
        return SCI_ERR_PARAM;
    }

    SCSMR1 = scsmr1;
    SCBRR1 = scbrr1;

    /* Allow time for the changes to take effect */
    spi_delay(10000);

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
        spi_delay(10000);
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
        SCFCR2 &= ~SCFCR_MCE;

        sptr = SCSPTR2;
        sptr &= ~(RTSIO | RTSDT);
        SCSPTR2 = sptr;
    }
    cs_mode = SCI_SPI_CS_NONE;
}

void sci_configure_spi(sci_spi_cs_mode_t cs) {
    uint16_t sptr;
    uint16_t fcr;

    sci_shutdown_spi_cs();

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

void sci_spi_set_cs(bool enabled) {
    uint16_t val;

    if(cs_mode == SCI_SPI_CS_GPIO) {
        val = PDTRA;

        if (enabled) {
            val &= ~SCI_SPI_CS_PDTRA_BIT;  /* Active CS = Low */
        }
        else {
            val |= SCI_SPI_CS_PDTRA_BIT;   /* Inactive CS = High */
        }

        PDTRA = val;
    }
    else if (cs_mode == SCI_SPI_CS_RTS) {
        val = SCSPTR2;

        if (enabled) {   
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
    byte = reverse_bits(b);

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

    timeout_cnt = 0;
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

    *data = reverse_bits(byte);

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
        byte = reverse_bits(*tx_data++);

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
        timeout_cnt = 0;
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

        *rx_data++ = reverse_bits(byte);
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
    byte = reverse_bits(b);

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

    *data = reverse_bits(byte);

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
        byte = reverse_bits(*tx_data++);

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

        *rx_data++ = reverse_bits(byte);
    }

    return SCI_OK;
}
