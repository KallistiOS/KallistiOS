/* KallistiOS ##version##

   fat/fs_fat.h
   Copyright (C) 2012, 2013, 2019 Lawrence Sebald
*/

/** \file    fat/fs_fat.h
    \brief   VFS interface for a FAT filesystem.
    \ingroup vfs_fat

    This file defines the public interface to add support for the FAT
    filesystem, as in common use on all kinds of systems and popularized by
    MS-DOS and Windows. This interface supports FAT12, FAT16, and FAT32, with
    both short and long names.

    Note that there is a lower-level interface sitting underneath of this layer.
    This lower-level interface (simply called fatfs) should not generally be
    used by any normal applications. As of this point, it is completely non
    thread-safe and the fs_fat layer takes extreme care to overcome those
    issues with the lower-level interface. Over time, I may fix the thread-
    safety issues in fatfs, but that is not particularly high on my priority
    list at the moment. There shouldn't really be a reason to work directly with
    the fatfs layer anyway, as this layer should give you everything you need
    by interfacing with the VFS in the normal fashion.

    \author Lawrence Sebald
*/

#ifndef __FAT_FS_FAT_H
#define __FAT_FS_FAT_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <kos/blockdev.h>

/** \defgroup vfs_fat   FAT
    \brief              FAT 12, 16, and 32-bit support for KOS's VFS
    \ingroup            vfs_drivers
*/

/** \brief   Initialize fs_fat.
    \ingroup vfs_fat

    This function initializes fs_fat, preparing various internal structures for
    use.

    \retval 0           On success. No error conditions currently defined.
*/
int fs_fat_init(void);

/** \brief   Shut down fs_fat.
    \ingroup vfs_fat

    This function shuts down fs_fat, basically undoing what fs_fat_init() did.

    \retval 0           On success. No error conditions currently defined.
*/
int fs_fat_shutdown(void);

/** \defgroup fat_mount_flags          Mount Flags
    \brief                             Mount flags for fs_fat
    \ingroup  vfs_fat

    These values are the valid flags that can be passed for the flags parameter
    to the fs_fat_mount() function. Note that these can be combined, except for
    the read-only flag.

    Also, it is not possible to mount some filesystems as read-write. For
    instance, if the filesystem was marked as not cleanly unmounted the driver
    will fail to mount the device as read-write. Also, if the block device does
    not support writing, then the filesystem will not be mounted as read-write
    (for obvious reasons).

    These should stay synchronized with the ones in fatfs.h.

    @{
*/
#define FS_FAT_MOUNT_READONLY       0x00000000  /**< \brief Mount read-only */
#define FS_FAT_MOUNT_READWRITE      0x00000001  /**< \brief Mount read-write */
/** @} */

/** \brief   Mount a FAT filesystem in the VFS.
    \ingroup vfs_fat

    This function mounts an fat filesystem to the specified mount point on the
    VFS. This function will detect whether or not an FAT filesystem exists on
    the given block device and mount it only if there is actually an FAT
    filesystem.

    \param  mp          The path to mount the filesystem at.
    \param  dev         The block device containing the filesystem.
    \param  flags       Mount flags. Bitwise OR of values from fat_mount_flags
    
    \retval 0           On success.
    \retval -1          On error.
*/
int fs_fat_mount(const char *mp, kos_blockdev_t *dev, uint32_t flags);

/** \brief   Unmount a FAT filesystem from the VFS.
    \ingroup vfs_fat

    This function unmoutns an FAT filesystem that was previously mounted by the
    fs_fat_mount() function.

    \param  mp          The mount point of the filesystem to be unmounted.
    
    \retval 0           On success.
    \retval -1          On error.
*/
int fs_fat_unmount(const char *mp);

/** \brief   Sync a FAT filesystem, flushing all pending writes to the block
             device.
    \ingroup vfs_fat

    This function completes all pending writes on the filesystem, making sure
    all data and metadata are in a consistent state on the block device. As both
    inode and block writes are normally postponed until they are either evicted
    from the cache or the filesystem is unmounted, doing this periodically may
    be a good idea if there is a chance that the filesystem will not be
    unmounted cleanly.

    \note   This function has no effect if the filesystem was mounted read-only.

    \param  mp          The mount point of the filesystem to be synced.
    
    \retval 0           On success.
    \retval -1          On error.
*/
int fs_fat_sync(const char *mp);

__END_DECLS
#endif /* !__FAT_FS_FAT_H */
