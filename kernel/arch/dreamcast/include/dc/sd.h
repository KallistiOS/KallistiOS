/* KallistiOS ##version##

   dc/sd.h
   Copyright (C) 2012 Lawrence Sebald
   Copyright (C) 2025 Ruslan Rostovtsev
*/

/** \file    dc/sd.h
    \brief   Block-level access to an SD card attached to the SCI or SCIF port.
    \ingroup vfs_sd

    This file contains the interface to working with SD card readers.
    The original SD card reader designed by jj1odm connects to the SCIF port,
    while the SCI port implementation was developed by SWAT (Ruslan Rostovtsev). 
    The SCIF implementation uses bit-banging technique to emulate SPI protocol,
    while the SCI implementation utilizes the synchronous mode of this interface,
    which is very similar to SPI.

    For reference, all I/O through this code should be done in the order of SD
    card blocks (which are 512 bytes a piece). Also, this should adequately
    support SD and SDHC cards (and possibly SDXC, but I don't have any of them
    to try out).

    This code doesn't directly implement any filesystems on top of the SD card,
    but rather provides you with direct block-level access. This probably will
    not be useful to most people in its current form (without a filesystem), but
    this will provide you with all of the building blocks you should need to
    actually make it work for you.

    Due to the patent-encumbered nature of certain parts of the FAT32
    filesystem, that filesystem will likely never be supported in KOS proper
    (unless, of course, people are still using KOS after those patents expire).
    I'm not going to encourage anyone to violate Microsoft's patents on FAT32
    and I'm not going to be the enabler for anyone to do so either. So, if you
    want FAT32, you're on your own.

    \author Lawrence Sebald
    \see    dc/scif.h
*/

#ifndef __DC_SD_H
#define __DC_SD_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <kos/blockdev.h>
#include <stdbool.h>

/** \defgroup vfs_sd    SD Card
    \brief              VFS driver for accessing SD cards over the SCIF or SCI port
    \ingroup            vfs

    @{
*/

/** \brief  SD card interface type */
typedef enum {
    SD_IF_SCIF = 0,    /**< Use SCIF interface */
    SD_IF_SCI = 1      /**< Use SCI interface */
} sd_interface_t;

/** \brief  SD card initialization parameters */
typedef struct {
    sd_interface_t interface;  /**< Interface to use (SCIF or SCI) */
    bool check_crc;           /**< Enable CRC checking (true) or disable (false) */
} sd_init_params_t;

/** \brief  Calculate a SD/MMC-style CRC over a block of data.

    This function calculates a 7-bit CRC over a given block of data. The
    polynomial of this CRC is x^7 + x^3 + 1. The CRC is shifted into the upper
    7 bits of the return value, so bit 0 should always be 0.

    \param  data            The block of data to calculate the CRC over.
    \param  size            The number of bytes in the block of data.
    \param  crc             The starting value of the calculation. If you're
                            passing in a full block, this will probably be 0.
    \return                 The calculated CRC.
*/
uint8 sd_crc7(const uint8 *data, int size, uint8 crc);

/** \brief  Initialize the SD card with extended parameters.

    This function initializes the SD card with specified parameters. This includes
    all steps of the basic initialization sequence for SPI mode, as documented in
    the SD card spec and at http://elm-chan.org/docs/mmc/mmc_e.html.

    \param  params          Pointer to initialization parameters.
    \retval 0               On success.
    \retval -1              On failure. This could indicate any number of
                            problems, but probably means that no SD card was
                            detected.
*/
int sd_init_ex(const sd_init_params_t *params);

/** \brief  Initialize the SD card for use.

    This function initializes the SD card for first use using default parameters
    (SCIF interface and CRC checking enabled).

    \retval 0               On success.
    \retval -1              On failure. This could indicate any number of
                            problems, but probably means that no SD card was
                            detected.
*/
int sd_init(void);

/** \brief  Shut down SD card support.

    This function shuts down SD card support, and cleans up anything that the
    sd_init() function set up.

    \retval 0               On success.
    \retval -1              On failure. The only currently defined error is if
                            the card was never initialized to start with.
*/
int sd_shutdown(void);

/** \brief  Read one or more blocks from the SD card.

    This function reads the specified number of blocks from the SD card from the
    beginning block specified into the buffer passed in. It is your
    responsibility to allocate the buffer properly for the number of bytes that
    is to be read (512 * the number of blocks requested).

    \param  block           The starting block number to read from.
    \param  count           The number of 512 byte blocks of data to read.
    \param  buf             The buffer to read into.
    \retval 0               On success.
    \retval -1              On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EIO - an I/O error occurred in reading data \n
    \em     ENXIO - SD card support was not initialized
*/
int sd_read_blocks(uint32 block, size_t count, uint8 *buf);

/** \brief  Write one or more blocks to the SD card.

    This function writes the specified number of blocks to the SD card at the
    beginning block specified from the buffer passed in. Each block is 512 bytes
    in length, and you must write at least one block at a time. You cannot write
    partial blocks.

    If this function returns an error, you have quite possibly corrupted
    something on the card or have a damaged card in general (unless errno is
    ENXIO).

    \param  block           The starting block number to write to.
    \param  count           The number of 512 byte blocks of data to write.
    \param  buf             The buffer to write from.
    \retval 0               On success.
    \retval -1              On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EIO - an I/O error occurred in reading data \n
    \em     ENXIO - SD card support was not initialized
*/
int sd_write_blocks(uint32 block, size_t count, const uint8 *buf);

/** \brief  Retrieve the size of the SD card.

    This function reads the size of the SD card from the card's CSD register.
    This is the raw size of the card, not its formatted capacity. To get the
    number of blocks from this, divide by 512.

    \return                 On success, the raw size of the SD card in bytes. On
                            error, (uint64)-1.

    \par    Error Conditions:
    \em     EIO - an I/O error occurred in reading data \n
    \em     ENXIO - SD card support was not initialized
*/
uint64 sd_get_size(void);

/** \brief  Get a block device for a given partition on the SD card.

    This function creates a block device descriptor for the given partition on
    the attached SD card. This block device is used to interface with various
    filesystems on the device.

    \param  partition       The partition number (0-3) to use.
    \param  rv              Used to return the block device. Must be non-NULL.
    \param  partition_type  Used to return the partition type. Must be non-NULL.
    \retval 0               On success.
    \retval -1              On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     ENXIO - SD card support was not initialized \n
    \em     EIO - an I/O error occurred in reading data \n
    \em     EINVAL - invalid partition number was given \n
    \em     EFAULT - rv or partition_type was NULL \n
    \em     ENOENT - no MBR found \n
    \em     ENOENT - no partition at the specified position \n
    \em     ENOMEM - out of memory

    \note   This interface currently only supports MBR-formatted SD cards. There
            is currently no support for GPT partition tables.
*/
int sd_blockdev_for_partition(int partition, kos_blockdev_t *rv,
                              uint8 *partition_type);

/** \brief  Get a block device for the SD card.

    This function creates a block device descriptor for the attached SD card.
    This block device is used to interface with various filesystems on the device.

    \param  rv              Used to return the block device. Must be non-NULL.
    \retval 0               On success.
    \retval -1              On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     ENXIO - SD card support was not initialized
    \em     EFAULT - rv was NULL
    \em     ENOMEM - out of memory
*/
int sd_blockdev_for_device(kos_blockdev_t *rv);

/** @} */

__END_DECLS
#endif /* !__DC_SD_H */
