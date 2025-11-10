/* KallistiOS ##version##

   dc/scif.h
   Copyright (C) 2000,2001,2004 Megan Potter
   Copyright (C) 2012 Lawrence Sebald
   Copyright (C) 2023 Ruslan Rostovtsev

*/

/** \file    dc/scif.h
    \brief   Serial port functionality.
    \ingroup system_scif

    This file deals with raw access to the serial port on the Dreamcast.

    \author Megan Potter
    \author Lawrence Sebald
    \author Ruslan Rostovtsev
*/

#ifndef __DC_SCIF_H
#define __DC_SCIF_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <kos/dbgio.h>

/** \defgroup system_scif   SCIF
    \brief                  Driver for managing the serial port
    \ingroup                system

    @{
*/

/** \brief  Default serial bitrate. */
#define DEFAULT_SERIAL_BAUD 115200

/** \brief  Default serial FIFO behavior. */
#define DEFAULT_SERIAL_FIFO 1

/** \brief  Set serial parameters.
    \param  baud            The bitrate to set.
    \param  fifo            1 to enable FIFO mode.
*/
void scif_set_parameters(int baud, int fifo);

/* The rest of these are the standard dbgio interface. */

/** \brief  Enable or disable SCIF IRQ usage.
    \param  on              1 to enable IRQ usage, 0 for polled I/O.
    \retval 0               On success (no error conditions defined).
*/
int scif_set_irq_usage(int on);

/** \brief  Is the SCIF port detected? Of course it is!
    \return                 1
*/
int scif_detected(void);

/** \brief  Initialize the SCIF port.

    This function initializes the SCIF port to a sane state. If dcload-serial is
    in use, this is effectively a no-op.

    \retval 0               On success (no error conditions defined).
*/
int scif_init(void);

/** \brief  Shutdown the SCIF port.

    This function disables SCIF IRQs, if they were enabled and cleans up.

    \retval 0               On success (no error conditions defined).
*/
int scif_shutdown(void);

/** \brief  Read a single character from the SCIF port.
    \return                 The character read if one is available, otherwise -1
                            and errno is set to EAGAIN.
*/
int scif_read(void);

/** \brief  Write a single character to the SCIF port.
    \param  c               The character to write (only the low 8-bits are
                            written).
    \retval 1               On success.
    \retval -1              If the SCIF port is disabled (errno set to EIO).
*/
int scif_write(int c);

/** \brief  Flush any FIFO'd bytes out of the buffer.

    This function sends any bytes that have been queued up for transmission but
    have not left yet in FIFO mode.

    \retval 0               On success.
    \retval -1              If the SCIF port is disabled (errno set to EIO).
*/
int scif_flush(void);

/** \brief  Write a whole buffer of data to the SCIF port.

    This function writes a whole buffer of data to the SCIF port, optionally
    making all newlines into carriage return + newline pairs.

    \param  data            The buffer to write.
    \param  len             The length of the buffer, in bytes.
    \param  xlat            If set to 1, all newlines will be written as CRLF.
    \return                 The number of bytes written on success, -1 on error.
*/
int scif_write_buffer(const uint8 *data, int len, int xlat);

/** \brief  Read a buffer of data from the SCIF port.

    This function reads a whole buffer of data from the SCIF port, blocking
    until it has been filled.

    \param  data            The buffer to read into.
    \param  len             The number of bytes to read.
    \return                 The number of bytes read on success, -1 on error.
*/
int scif_read_buffer(uint8 *data, int len);

/** \brief  SCIF debug I/O handler. Do not modify! */
extern dbgio_handler_t dbgio_scif;

/* Low-level SPI related functionality below here... */
/** \brief  Initialize the SCIF port for use of an SPI peripheral.

    This function initializes the SCIF port for accessing the an SPI peripheral
    that has been connected to the serial port. The design of the SCIF->SPI
    wiring follows the wiring of the SD card adapter which is (at least now)
    somewhat commonly available online and is the same as the one designed by
    jj1odm.

    \retval 0               On success.
    \retval -1              On error (if dcload-serial is detected).
*/
int scif_spi_init(void);

/** \brief  Shut down SPI card support over the SCIF port.

    This function shuts down SPI support on the SCIF port. If you want to get
    regular usage of the port back, you must call scif_init() after shutting
    down SPI support.

    \retval 0               On success (no errors defined).
*/
int scif_spi_shutdown(void);

/** \brief  Set or clear the SPI /CS line.

    This function sets or clears the /CS line (connected to the RTS line of the
    SCIF port).

    \param  v               Non-zero to output 1 on the line, zero to output 0.
*/
void scif_spi_set_cs(int v);

/** \brief  Read and write one byte from the SPI port.

    This function writes one byte and reads one back from the SPI device
    simultaneously.

    \param  b               The byte to write out to the port.
    \return                 The byte returned from the card.
*/
uint8 scif_spi_rw_byte(uint8 b);

/** \brief  Read and write one byte from the SPI device, slowly.

    This function does the same thing as the scif_sd_rw_byte() function, but
    with a 1.5usec delay between asserting the CLK line and reading back the bit
    and a 1.5usec delay between clearing the CLK line and writing the next bit
    out.

    This ends up working out to a clock of about 333khz, or so.

    \param  b               The byte to write out to the port.
    \return                 The byte returned from the card.
*/
uint8 scif_spi_slow_rw_byte(uint8 b);


/** \brief  Write a byte to the SPI device.

    This function writes out the specified byte to the SPI device, one bit at a
    time. The timing follows that of the scif_spi_rw_byte() function.

    \param  b               The byte to write out to the port.
*/
void scif_spi_write_byte(uint8 b);

/** \brief  Read a byte from the SPI device.

    This function reads a byte from the SPI device, one bit at a time. Timing
    is similar to (but slightly faster than) the scif_spi_rw_byte() function.

    \return                 The byte returned from the device.
*/
uint8 scif_spi_read_byte(void);

/** \brief  Read a data from the SPI device.

    This function reads data from the SPI device. If the buffer is aligned and
    len is divisible by 4, optimizations are applied.

    \param  buffer          Buffer to store read data into.
    \param  len             Number of bytes to read from the device.
*/
void scif_spi_read_data(uint8 *buffer, size_t len);

/** @} */

__END_DECLS

#endif  /* __DC_SCIF_H */
