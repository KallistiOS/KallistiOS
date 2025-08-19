/* KallistiOS ##version##

   kos/blockdev.h
   Copyright (C) 2012, 2013 Lawrence Sebald
*/

/** \file    kos/blockdev.h
    \brief   Definitions for a simple block device interface.
    \ingroup vfs_blockdev

    This file contains the definition of a very simple block device that is to
    be used with filesystems in the kernel. This device interface is designed to
    abstract away direct hardware access and make it easier to interface the
    various filesystems that we may add support for to multiple potential
    devices.

    The most common of these devices that people are probably interested in
    directly would be the Dreamcast SD card reader, and that was indeed the
    primary impetus to this device structure. However, it could also be used
    to support a file-based disk image or any number of other devices.

    \author Lawrence Sebald
*/

#ifndef __KOS_BLOCKDEV_H
#define __KOS_BLOCKDEV_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <sys/types.h>

/** \defgroup vfs_blockdev  Block Devices
    \brief                  VFS driver for accessing block devices
    \ingroup                vfs_drivers

    @{
*/

/** \brief  A simple block device.

    This structure represents a single block device. Each block device should be
    associated with exactly one filesystem and is used to actually read the data
    from the disk (or other device) where it is stored.

    By using a block device with any new filesystems, we can abstract away a few
    things so that filesystems can be used with a variety of different
    "devices", such as the SD card reader for the Dreamcast or a disk image file
    of some sort.

    \headerfile kos/blockdev.h
*/
typedef struct kos_blockdev {
    void *dev_data;         /**< \brief Internal device data. */
    uint32_t l_block_size;  /**< \brief Log base 2 of the bytes per block. */

    /** \brief  Initialize the block device.

        This function should do any necessary initialization to use the block
        device passed in.

        \param  d           The device to initialize.
        \retval 0           On success.
        \retval -1          On failure. Set errno as appropriate.
    */
    int (*init)(struct kos_blockdev *d);

    /** \brief  Shut down the block device.

        This function should do any teardown work that is needed to clean up
        the block device.

        \param  d           The device to shut down.
        \retval 0           On success.
        \retval -1          On failure. Set errno as appropriate.
    */
    int (*shutdown)(struct kos_blockdev *d);

    /** \brief  Read a number of blocks from the device.

        This function should read the specified number of device blocks into
        the given buffer. The buffer will already be allocated by the caller.

        \param  d           The device to read from.
        \param  block       The first block to read.
        \param  count       The number of blocks to read.
        \param  buf         The buffer to read into.
        \retval 0           On success.
        \retval -1          On failure. Set errno as appropriate.
    */
    int (*read_blocks)(struct kos_blockdev *d, uint64_t block, size_t count,
                       void *buf);

    /** \brief  Write a number of blocks to the device.

        This function should write the specified number of device blocks onto
        the device from the given buffer.

        \param  d           The device to write to.
        \param  block       The first block to write.
        \param  count       The number of blocks to write.
        \param  buf         The buffer to write from.
        \retval 0           On success.
        \retval -1          On failure. Set errno as appropriate.
    */
    int (*write_blocks)(struct kos_blockdev *d, uint64_t block, size_t count,
                        const void *buf);

    /** \brief  Count the number of blocks on the device.

        This function should return the total number of blocks on the device.
        There is no expectation of the device to keep track of which blocks are
        in use or anything else of the sort.

        \param  d           The device to read the block count from.
        \return             The number of blocks that the device has.
    */
    uint64_t (*count_blocks)(struct kos_blockdev *d);

    /** \brief  Flush the write cache (if any) of the device.

        This function shall signal to the device that any write caches that are
        present on the device shall be flushed so that all data written to this
        point shall persist to the underlying storage.

        \param  d           The device to flush caches on.
        \retval 0           On success.
        \retval -1          On failure. Set errno as appropriate.
    */
    int (*flush)(struct kos_blockdev *d);
} kos_blockdev_t;

/** @} */

__END_DECLS

#endif /* !__KOS_BLOCKDEV_H */
