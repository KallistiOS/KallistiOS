/* KallistiOS ##version##

   dc/sci.h
   Copyright (C) 2025 Ruslan Rostovtsev

*/

/** \file    dc/sci.h
    \brief   Serial Communication Interface functionality.
    \ingroup system_sci

    This file provides access to the Dreamcast/Naomi SCI module, which can operate
    in both UART and SPI modes.

    \author Ruslan Rostovtsev
*/

#ifndef __DC_SCI_H
#define __DC_SCI_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <stdint.h>
#include <stdbool.h>

/** \defgroup system_sci    SCI
    \brief                  Driver for the Serial Communication Interface
    \ingroup                system

    @{
*/

/*
 * Baudrate configuration for SH7750 SCI (PCLK = 50 MHz)
 *
 * The maximum achievable baudrate depends on internal divider settings.
 *
 * Asynchronous mode (UART):
 *   Formula: BRR = (PCLK / (64 × 2^(2n-1) × Baudrate)) - 1, where n = 0–3 (CKS bits)
 *   Maximum with internal clock (n=0, BRR=0): ~1,562,500 bps
 *
 * Synchronous mode (SPI):
 *   Formula: BRR = (PCLK / (8 × 2^(2n-1) × Baudrate)) - 1, where n = 0–3 (CKS bits)
 *   Maximum with internal clock (n=0, BRR=0): ~12.5 Mbps
 */

/** \brief Preset baudrates for UART mode of SCI (internal clock) */
#define SCI_UART_BAUD_4800     4800        /* Error < 0.2%, very reliable */
#define SCI_UART_BAUD_9600     9600        /* Error < 0.2%, very reliable */
#define SCI_UART_BAUD_19200    19200       /* Error < 0.5%, very reliable */
#define SCI_UART_BAUD_38400    38400       /* Error < 0.8%, very reliable */
#define SCI_UART_BAUD_57600    57600       /* Error < 0.5%, very reliable */
#define SCI_UART_BAUD_76800    76800       /* Error ~1.7%, reliable */
#define SCI_UART_BAUD_115200   115200      /* Error ~3.1%, acceptable */
#define SCI_UART_BAUD_230400   230400      /* Error ~3.1%, acceptable */
#define SCI_UART_BAUD_256000   256000      /* Error ~1.7%, reliable */
#define SCI_UART_BAUD_312500   312500      /* Perfect match, error 0% */
#define SCI_UART_BAUD_390625   390625      /* Perfect match, error 0% */
#define SCI_UART_BAUD_460800   460800      /* Error ~13.0%, not recommended */
#define SCI_UART_BAUD_576000   576000      /* Error < 0.5%, very reliable */
#define SCI_UART_BAUD_781250   781250      /* Perfect match, error 0% */
#define SCI_UART_BAUD_921600   921600      /* Error ~15.2%, not recommended */
#define SCI_UART_BAUD_1562500  1562500     /* Theoretical max, n=0, BRR=0, error 0% */

/** \brief Preset baudrates for SPI mode of SCI (internal clock) */
#define SCI_SPI_BAUD_250K      250000      /* Perfect match, error 0% */
#define SCI_SPI_BAUD_312K      312500      /* Perfect match, error 0% */
#define SCI_SPI_BAUD_500K      500000      /* Perfect match, error 0% */
#define SCI_SPI_BAUD_625K      625000      /* Perfect match, error 0% */
#define SCI_SPI_BAUD_781K      781250      /* Perfect match, error 0% */
#define SCI_SPI_BAUD_1M562K    1562500     /* Perfect match, error 0% */
#define SCI_SPI_BAUD_3M125K    3125000     /* Perfect match, error 0% */
#define SCI_SPI_BAUD_6M250K    6250000     /* Perfect match, error 0% */
#define SCI_SPI_BAUD_12M500K   12500000    /* Theoretical max, n=0, BRR=0, error 0% */

/** \brief Default baudrate for SPI mode */
#define SCI_SPI_BAUD_INIT   SCI_SPI_BAUD_312K     /* Initialization baudrate for SPI */
#define SCI_SPI_BAUD_MAX    SCI_SPI_BAUD_12M500K  /* Maximum baudrate for SPI */

/** \brief SCI operating mode definitions */
typedef enum {
    SCI_MODE_NONE = -1,  /**< No mode selected */
    SCI_MODE_UART = 0,   /**< UART (asynchronous) mode */
    SCI_MODE_SPI = 1     /**< SPI (synchronous) mode */
} sci_mode_t;

/** \brief Clock source options */
typedef enum {
    SCI_CLK_INT = 0,     /**< Use internal clock (default) */
    SCI_CLK_EXT = 1      /**< Use external clock (SCK pin) */
} sci_clock_t;

/** \brief UART configuration options */
typedef enum {
    SCI_UART_8N1 = 0x00,  /**< 8 data bits, no parity, 1 stop bit */
    SCI_UART_8N2 = 0x08,  /**< 8 data bits, no parity, 2 stop bits */
    SCI_UART_8E1 = 0x10,  /**< 8 data bits, even parity, 1 stop bit */
    SCI_UART_8O1 = 0x30,  /**< 8 data bits, odd parity, 1 stop bit */
    SCI_UART_7N1 = 0x40,  /**< 7 data bits, no parity, 1 stop bit */
    SCI_UART_7N2 = 0x48,  /**< 7 data bits, no parity, 2 stop bits */
    SCI_UART_7E1 = 0x50,  /**< 7 data bits, even parity, 1 stop bit */
    SCI_UART_7O1 = 0x70   /**< 7 data bits, odd parity, 1 stop bit */
} sci_uart_config_t;

/** \brief SPI CS pin options */
typedef enum {
    SCI_SPI_CS_NONE = -1, /**< No CS mode selected */
    SCI_SPI_CS_GPIO = 0,  /**< Use GPIO for SPI CS */
    SCI_SPI_CS_RTS = 1    /**< Use RTS from SCIF for SPI CS */
} sci_spi_cs_mode_t;

/** \brief Error codes for SCI operations */
typedef enum {
    SCI_OK = 0,                     /**< No error */
    SCI_ERR_NOT_INITIALIZED = -1,   /**< Not initialized or incorrect mode */
    SCI_ERR_PARAM = -2,             /**< Invalid parameter */
    SCI_ERR_TIMEOUT = -3,           /**< Operation timeout */
    SCI_ERR_OVERRUN = -4,           /**< Overrun error */
    SCI_ERR_FRAMING = -5,           /**< Framing error */
    SCI_ERR_PARITY = -6,            /**< Parity error */
} sci_result_t;

/** \brief  Initialize the SCI port with specified parameters.
    \param  baud_rate       The baudrate to set.
    \param  mode            SCI_MODE_UART for UART mode, SCI_MODE_SPI for SPI mode.
    \param  clock_src       Clock source (internal or external).
    \return                 SCI_OK on success, error code otherwise.
*/
sci_result_t sci_init(uint32_t baud_rate, sci_mode_t mode, sci_clock_t clock_src);

/** \brief  Configure UART parameters.
    \param  config          UART configuration (data bits, parity, stop bits).
    \param  scsmr1          Pointer to store configuration value if needed.
*/
void sci_configure_uart(sci_uart_config_t config, uint8_t *scsmr1);

/** \brief  Configure SPI parameters.
    \param  cs              Chip select mode for SPI.
*/
void sci_configure_spi(sci_spi_cs_mode_t cs);

/** \brief  Shutdown the SCI port.
*/
void sci_shutdown();

/** \brief  Read a single byte from the UART.
    \param  data            Pointer to store the read byte.
    \return                 SCI_OK on success, error code otherwise.
*/
sci_result_t sci_read_byte(uint8_t *data);

/** \brief  Write a single byte to the UART.
    \param  data            The byte to write.
    \return                 SCI_OK on success, error code otherwise.
*/
sci_result_t sci_write_byte(uint8_t data);

/** \brief  Write multiple bytes to the UART.
    \param  data            Buffer containing data to write.
    \param  len             Number of bytes to write.
    \return                 SCI_OK on success, error code otherwise.
*/
sci_result_t sci_write_data(uint8_t *data, size_t len);

/** \brief  Read multiple bytes from the UART.
    \param  data            Buffer to store read data.
    \param  len             Number of bytes to read.
    \return                 SCI_OK on success, error code otherwise.
*/
sci_result_t sci_read_data(uint8_t *data, size_t len);

/** \brief  Set or clear the SPI chip select line.
    \param  enabled         true to assert CS (active low), false to deassert.
*/
void sci_spi_set_cs(bool enabled);

/** \brief  Read and write one byte to the SPI device simultaneously.
    \param  tx_byte         The byte to write out to the device.
    \param  rx_byte         Pointer to store the received byte.
    \return                 SCI_OK on success, error code otherwise.
*/
sci_result_t sci_spi_rw_byte(uint8_t tx_byte, uint8_t *rx_byte);

/** \brief  Read and write multiple bytes to/from the SPI device.
    \param  tx_data         Buffer containing data to write.
    \param  rx_data         Buffer to store received data.
    \param  len             Number of bytes to transfer.
    \return                 SCI_OK on success, error code otherwise.
*/
sci_result_t sci_spi_rw_data(const uint8_t *tx_data, uint8_t *rx_data, size_t len);

/** \brief  Write one byte to the SPI device.
    \param  tx_byte         The byte to write out to the device.
    \return                 SCI_OK on success, error code otherwise.
*/
sci_result_t sci_spi_write_byte(uint8_t tx_byte);

/** \brief  Read one byte from the SPI device.
    \param  rx_byte         Pointer to store the received byte.
    \return                 SCI_OK on success, error code otherwise.
*/
sci_result_t sci_spi_read_byte(uint8_t *rx_byte);

/** \brief  Write multiple bytes to the SPI device.
    \param  tx_data         Buffer containing data to write.
    \param  len             Number of bytes to transfer.
    \return                 SCI_OK on success, error code otherwise.
*/
sci_result_t sci_spi_write_data(const uint8_t *tx_data, size_t len);

/** \brief  Read multiple bytes from the SPI device.
    \param  rx_data         Buffer to store received data.
    \param  len             Number of bytes to transfer.
    \return                 SCI_OK on success, error code otherwise.
*/
sci_result_t sci_spi_read_data(uint8_t *rx_data, size_t len);

/** @} */

__END_DECLS

#endif  /* __DC_SCI_H */
