/* KallistiOS ##version##

   dc/g1ata.h
   Copyright (C) 2013, 2014 Lawrence Sebald
   Copyright (C) 2023, 2024 Ruslan Rostovtsev
*/

/** \file    dc/g1ata.h
    \brief   G1 bus ATA interface.
    \ingroup g1ata

    This file provides support for accessing an ATA device on the G1 bus in the
    Dreamcast. The G1 bus usually contains a few useful pieces of the system,
    including the flashrom and the GD-ROM drive. The interesting piece here is
    that the GD-ROM drive itself is actually an ATA device.

    Luckily, Sega left everything in place to access both a master and slave
    device on this ATA port. The GD-ROM drive should always be the master device
    on the chain, but you can hook up a hard drive or some other device as a
    slave. The functions herein are for accessing just such a slave device.

    \note   The functions herein do not provide for direct access to the GD-ROM
            drive. There is not really any sort of compelling reason to access
            the GD-ROM drive directly instead of using the system calls, so you
            should continue to use the normal cdrom_* functions for accessing
            the GD-ROM drive. Also, currently there is no locking done to
            prevent you from doing "bad things" with concurrent access on the
            bus, so be careful. ;)

    \author Lawrence Sebald
    \author Ruslan Rostovtsev
*/

#ifndef __DC_G1ATA_H
#define __DC_G1ATA_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <kos/blockdev.h>

/** \defgroup g1ata G1 ATA
    \brief          Driver for Accessing an ATA device on the G1 Bus
    \ingroup        vfs_drivers
*/

/** \defgroup ata_devices   Device Definitions
    \brief                  ATA device definitions
    \ingroup                g1ata

    The constants here represent the valid values that can be set as the active
    device on the ATA bus. You should pass one of these values to the
    g1_ata_select_device() function to select the appropriate device.

    \note   Many times, the value returned by the g1_ata_select_device()
            function will have other bits set than the constants below. You
            should AND the value returned from that function with 0x10 if you
            really need to know what device is actually selected. If the value
            returned is true when ANDed with 0x10, then the slave device was
            selected, otherwise the master device was. The other bits are either
            command-specific or are reserved for compatibility sake.

    @{
*/
/** \brief   ATA master device.
    \ingroup g1ata

    This constant selects the master device on the ATA bus. This is normally the
    GD-ROM drive.

    \note   The GD-ROM really does not like the reserved bits being set in the
            device select register, hence why this constant doesn't select them.
            Some hard drives may require them, however. If you find one that
            does, then you should use the \ref G1_ATA_MASTER_ALT constant to
            access it if it is the master device on the bus.
*/
#define G1_ATA_MASTER       0x00

/** \brief   ATA master device (compatible with old drives).
    \ingroup g1ata

    This constant selects the master device on the ATA bus, with the old
    reserved bits set to 1. If you have a drive that predates ATA-2, then this
    will probably be the constant you want to access it as the master device.

    \note   Do not use this constant to access the GD-ROM. It will not work. Use
            \ref G1_ATA_MASTER instead.
*/
#define G1_ATA_MASTER_ALT   0x90

/** \brief   ATA slave device.
    \ingroup g1ata

    This constant selects the slave device on the ATA bus. This is where you
    would find a hard drive, if the user has an adapter installed.
*/
#define G1_ATA_SLAVE        0xB0

/** \brief   Select LBA addressing mode.
    \ingroup g1ata

    OR this constant with one of the device constants (\ref G1_ATA_MASTER or
    \ref G1_ATA_SLAVE) to select LBA addressing mode. The various g1_ata_*
    functions all do this as appropriate already, so you shouldn't have to worry
    about this one at all. This bit is irrelevant for packet devices.
*/
#define G1_ATA_LBA_MODE     0x40
/** @} */

/** \brief   Is there a G1 DMA in progress currently?
    \ingroup g1ata

    This function returns non-zero if a DMA is in progress. This can be used to
    check on the completion of DMA transfers when non-blocking mode was selected
    at transfer time.

    \return                 0 if no DMA is in progress, nonzero otherwise.
*/
int g1_dma_in_progress(void);

/** \brief   Lock the G1 ATA mutex.
    \ingroup g1ata

    This function locks the mutex that arbitrates access to the ATA bus. You
    should never have to do this on your own unless you're accessing devices
    manually yourself.

    \return                 0 on success, -1 on failure.
    \note                   Failure conditions are exactly the same as the
                            \ref mutex_lock() function.
*/
int g1_ata_mutex_lock(void);

/** \brief   Unlock the G1 ATA mutex.
    \ingroup g1ata

    This function unlocks the mutex that arbitrates access to the ATA bus. You
    should never have to do this on your own unless you're accessing devices
    manually yourself.

    \return                 0 on success, -1 on failure.
    \note                   Failure conditions are exactly the same as the
                            \ref mutex_unlock() function.
*/
int g1_ata_mutex_unlock(void);

/** \brief   Set the active ATA device.
    \ingroup g1ata

    This function sets the device that any further ATA commands will go to. You
    shouldn't have to ever call this yourself, as it should be done for you by
    any of the access functions. This must be called with the ATA lock held.

    \param  dev             The device to access (generally either
                            \ref G1_ATA_MASTER or \ref G1_ATA_SLAVE).
    \return                 The previous active device (or 0x0F if the function
                            would block in an IRQ handler).

    \note                   This function may block if there is a transfer
                            ongoing. If called in an IRQ handler and the call
                            would otherwise block, 0x0F is returned.
*/
uint8_t g1_ata_select_device(uint8_t dev);

/** \brief   Read one or more disk sectors with Cylinder-Head-Sector addressing.
    \ingroup g1ata

    This function reads one or more 512-byte disk blocks from the slave device
    on the G1 ATA bus using Cylinder-Head-Sector addressing. This function uses
    PIO and blocks until the data is read in.

    \param  c               The cylinder to start reading from.
    \param  h               The head to start reading from.
    \param  s               The sector to start reading from.
    \param  count           The number of disk sectors to read.
    \param  buf             Storage for the read-in disk sectors. This should be
                            at least (count * 512) bytes in length, and must be
                            at least 16-bit aligned.
    \return                 0 on success. < 0 on failure, setting errno as
                            appropriate.

    \note                   Unless you're accessing a really old hard drive, you
                            probably do not want to use this function to access
                            the disk. Use the g1_ata_read_lba() function instead
                            of this one, unless you get an error from that
                            function indicating that LBA addressing is not
                            supported.

    \par    Error Conditions:
    \em     EIO - an I/O error occurred in reading data \n
    \em     ENXIO - ATA support not initialized or no device attached \n
    \em     EOVERFLOW - one or more of the requested sectors is out of the
                        range of the disk
*/
int g1_ata_read_chs(uint16_t c, uint8_t h, uint8_t s, size_t count,
                    void *buf);

/** \brief   Write one or more disk sectors with Cylinder-Head-Sector addressing.
    \ingroup g1ata

    This function writes one or more 512-byte disk blocks to the slave device
    on the G1 ATA bus using Cylinder-Head-Sector addressing. This function uses
    PIO and blocks until the data is written.

    \param  c               The cylinder to start writing to.
    \param  h               The head to start writing to.
    \param  s               The sector to start writing to.
    \param  count           The number of disk sectors to write.
    \param  buf             The data to write to the disk. This should be
                            (count * 512) bytes in length and must be at least
                            16-bit aligned.
    \return                 0 on success. < 0 on failure, setting errno as
                            appropriate.

    \note                   Unless you're accessing a really old hard drive, you
                            probably do not want to use this function to access
                            the disk. Use the g1_ata_write_lba() function
                            instead of this one, unless you get an error from
                            that function indicating that LBA addressing is not
                            supported.

    \par    Error Conditions:
    \em     ENXIO - ATA support not initialized or no device attached \n
    \em     EOVERFLOW - one or more of the requested sectors is out of the
                        range of the disk
*/
int g1_ata_write_chs(uint16_t c, uint8_t h, uint8_t s, size_t count,
                     const void *buf);

/** \brief   Read one or more disk sectors with Linear Block Addressing (LBA).
    \ingroup g1ata

    This function reads one or more 512-byte disk blocks from the slave device
    on the G1 ATA bus using LBA mode (either 28 or 48 bits, as appropriate).
    This function uses PIO and blocks until the data is read.

    \param  sector          The sector to start reading from.
    \param  count           The number of disk sectors to read.
    \param  buf             Storage for the read-in disk sectors. This should be
                            at least (count * 512) bytes in length, and must be
                            at least 16-bit aligned.
    \return                 0 on success. < 0 on failure, setting errno as
                            appropriate.

    \note                   If errno is set to ENOTSUP after calling this
                            function, you must use the g1_ata_read_chs()
                            function instead.

    \par    Error Conditions:
    \em     EIO - an I/O error occurred in reading data \n
    \em     ENXIO - ATA support not initialized or no device attached \n
    \em     EOVERFLOW - one or more of the requested sectors is out of the
                        range of the disk \n
    \em     ENOTSUP - LBA mode not supported by the device
*/
int g1_ata_read_lba(uint64_t sector, size_t count, void *buf);

/** \brief   DMA read disk sectors with Linear Block Addressing (LBA).
    \ingroup g1ata

    This function reads one or more 512-byte disk blocks from the slave device
    on the G1 ATA bus using LBA mode (either 28 or 48 bits, as appropriate).
    This function uses DMA and optionally blocks until the data is read.

    \param  sector          The sector to start reading from.
    \param  count           The number of disk sectors to read.
    \param  buf             Storage for the read-in disk sectors. This should be
                            at least (count * 512) bytes in length, and must be
                            at least 32-byte aligned.
    \param  block           Non-zero to block until the transfer completes.
    \return                 0 on success. < 0 on failure, setting errno as
                            appropriate.

    \note                   If errno is set to ENOTSUP after calling this
                            function, you must use a CHS addressed transfer
                            function instead, like g1_ata_read_chs().

    \note                   If errno is set to EPERM after calling this
                            function, DMA mode is not supported. You should use
                            a PIO transfer function like g1_ata_read_lba()
                            instead.

    \note                   If the buffer address points to the P2 memory area,
                            the caller function will be responsible for ensuring
                            memory coherency.

    \par    Error Conditions:
    \em     EIO - an I/O error occurred in reading data \n
    \em     ENXIO - ATA support not initialized or no device attached \n
    \em     EOVERFLOW - one or more of the requested sectors is out of the
                        range of the disk \n
    \em     ENOTSUP - LBA mode not supported by the device \n
    \em     EPERM - device does not support DMA
*/
int g1_ata_read_lba_dma(uint64_t sector, size_t count, void *buf,
                        int block);

/** \brief   Write one or more disk sectors with Linear Block Addressing (LBA).
    \ingroup g1ata

    This function writes one or more 512-byte disk blocks to the slave device
    on the G1 ATA bus using LBA mode (either 28 or 48 bits, as appropriate).
    This function uses PIO and blocks until the data is written.

    \param  sector          The sector to start writing to.
    \param  count           The number of disk sectors to write.
    \param  buf             The data to write to the disk. This should be
                            (count * 512) bytes in length and must be at least
                            16-bit aligned.
    \return                 0 on success. < 0 on failure, setting errno as
                            appropriate.

    \note                   If errno is set to ENOTSUP after calling this
                            function, you must use the g1_ata_write_chs()
                            function instead.

    \par    Error Conditions:
    \em     ENXIO - ATA support not initialized or no device attached \n
    \em     EOVERFLOW - one or more of the requested sectors is out of the
                        range of the disk \n
    \em     ENOTSUP - LBA mode not supported by the device
*/
int g1_ata_write_lba(uint64_t sector, size_t count, const void *buf);

/** \brief   DMA Write disk sectors with Linear Block Addressing (LBA).
    \ingroup g1ata

    This function writes one or more 512-byte disk blocks to the slave device
    on the G1 ATA bus using LBA mode (either 28 or 48 bits, as appropriate).
    This function uses DMA and optionally blocks until the data is written.

    \param  sector          The sector to start writing to.
    \param  count           The number of disk sectors to write.
    \param  buf             The data to write to the disk. This should be
                            (count * 512) bytes in length and must be at least
                            32-byte aligned.
    \param  block           Non-zero to block until the transfer completes.
    \return                 0 on success. < 0 on failure, setting errno as
                            appropriate.

    \note                   If errno is set to ENOTSUP after calling this
                            function, you must use the g1_ata_write_chs()
                            function instead.

    \note                   If errno is set to EPERM after calling this
                            function, DMA mode is not supported. You should use
                            a PIO transfer function like g1_ata_write_lba()
                            instead.

    \note                   If the buffer address points to the P2 memory area,
                            the caller function will be responsible for ensuring
                            memory coherency.

    \par    Error Conditions:
    \em     ENXIO - ATA support not initialized or no device attached \n
    \em     EOVERFLOW - one or more of the requested sectors is out of the
                        range of the disk \n
    \em     ENOTSUP - LBA mode not supported by the device \n
    \em     EPERM - device does not support DMA
*/
int g1_ata_write_lba_dma(uint64_t sector, size_t count, const void *buf,
                         int block);

/** \brief   Flush the write cache on the attached disk.
    \ingroup g1ata

    This function flushes the write cache on the disk attached as the slave
    device on the G1 ATA bus. This ensures that all writes that have previously
    completed are fully persisted to the disk. You should do this before
    unmounting any disks or exiting your program if you have called any of the
    write functions in here.

    \return                 0 on success. <0 on error, setting errno as
                            appropriate.

    \par    Error Conditions:
    \em     ENXIO - ATA support not initialized or no device attached
*/
int g1_ata_flush(void);

/** \brief   Get LBA mode of the attached disk.
    \ingroup g1ata

    \return                 -1 on error, 0 - CHS, 28 - LBA28, 48 - LBA48

    \par    Error Conditions:
    \em     ENXIO - ATA support not initialized or no device attached
*/
int g1_ata_lba_mode(void);

/** \brief   Get a block device for a given partition on the slave ATA device.
    \ingroup g1ata

    This function creates a block device descriptor for the given partition on
    the attached ATA device. This block device is used to interface with various
    filesystems on the device.

    \param  partition       The partition number (0-3) to use.
    \param  dma             Set to 1 to use DMA for reads/writes on the device,
                            if available.
    \param  rv              Used to return the block device. Must be non-NULL.
    \param  partition_type  Used to return the partition type. Must be non-NULL.
    \retval 0               On success.
    \retval -1              On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     ENXIO - ATA support not initialized or no device attached \n
    \em     EIO - an I/O error occurred in reading data \n
    \em     EINVAL - invalid partition number was given \n
    \em     EFAULT - rv or partition_type was NULL \n
    \em     ENOENT - no MBR found \n
    \em     ENOENT - no partition at the specified position \n
    \em     ENOMEM - out of memory

    \note   This interface currently only supports MBR-formatted disks. There
            is currently no support for GPT partition tables.
*/
int g1_ata_blockdev_for_partition(int partition, int dma, kos_blockdev_t *rv,
                                  uint8_t *partition_type);

/** \brief   Get a block device for the attached ATA device.
    \ingroup g1ata

    This function creates a block device descriptor for the attached ATA device.

    \param  dma             Set to 1 to use DMA for reads/writes on the device,
                            if available.
    \param  rv              Used to return the block device. Must be non-NULL.
    \retval 0               On success.
    \retval -1              On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     ENXIO - ATA support not initialized or no device attached \n
    \em     EFAULT - rv was NULL \n
    \em     ENOMEM - out of memory
*/
int g1_ata_blockdev_for_device(int dma, kos_blockdev_t *rv);

/** \brief   Initialize G1 ATA support.
    \ingroup g1ata

    This function initializes the rest of this subsystem and completes a scan of
    the G1 ATA bus for devices. This function may take a while to complete with
    some devices. Currently only the slave device is scanned, as the master
    device should always be the GD-ROM drive.

    \return                 0 on success, <0 on error or if no device is present
*/
int g1_ata_init(void);

/** \brief   Shut down G1 ATA support.
    \ingroup g1ata

    This function shuts down the rest of this subsystem, and attempts to flush
    the write cache of any attached slave devices. Accessing any ATA devices
    using this subsystem after this function is called may produce undefined
    results.
*/
void g1_ata_shutdown(void);

__END_DECLS

#endif /* __DC_G1ATA_H */
