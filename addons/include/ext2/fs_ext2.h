/* KallistiOS ##version##

   ext2/fs_ext2.h
   Copyright (C) 2012, 2013 Lawrence Sebald
*/

/** \file    ext2/fs_ext2.h
    \brief   VFS interface for an ext2 filesystem.
    \ingroup vfs_ext2

    This file defines the public interface to add support for the Second
    Extended Filesystem (ext2) to KOS' VFS. ext2 is one of the many filesystems
    that is natively supported by Linux, and was the main filesystem used by
    most Linux installations pretty much until the creation of the ext3
    filesystem.

    The KOS ext2 driver was designed with two purposes. First of all, this fs
    was added to provide a filesystem for use on SD cards used with the
    Dreamcast SD adapter. ext2 was chosen for this purpose for a bunch of
    reasons, but probably the biggest one was the non-patent-encumbered nature
    of ext2 and the availability of programs/drivers to read ext2 on most major
    OSes available for PCs today. The second purpose of this filesystem driver
    is to provide an alternative for fs_romdisk when swapping out disk images at
    runtime. Basically, if a disk image is useful to you, but caching it fully
    in memory is not important, then you could rig up a relatively simple
    interface with this filesystem driver.

    Note that there is a lower-level interface sitting underneath of this layer.
    This lower-level interface (simply called ext2fs) should not generally be
    used by any normal applications. As of this point, it is completely non
    thread-safe and the fs_ext2 layer takes extreme care to overcome those
    issues with the lower-level interface. Over time, I may fix the thread-
    safety issues in ext2fs, but that is not particularly high on my priority
    list at the moment. There shouldn't really be a reason to work directly with
    the ext2fs layer anyway, as this layer should give you everything you need
    by interfacing with the VFS in the normal fashion.

    There's one final note that I should make. Everything in fs_ext2 and ext2fs
    is licensed under the same license as the rest of KOS. None of it was
    derived from GPLed sources. Pretty much all of what's in ext2fs was written
    based on the documentation at http://www.nongnu.org/ext2-doc/ .
    
    \author Lawrence Sebald
*/

#ifndef __EXT2_FS_EXT2_H
#define __EXT2_FS_EXT2_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <kos/blockdev.h>

/** \defgroup vfs_ext2  EXT2 
    \brief              KOS VFS support for the Second Extended Filesystem
    \ingroup            vfs_drivers
*/

/** \brief   Initialize fs_ext2.
    \ingroup vfs_ext2

    This function initializes fs_ext2, preparing various internal structures for
    use.

    \retval 0           On success. No error conditions currently defined.
*/
int fs_ext2_init(void);

/** \brief   Shut down fs_ext2.
    \ingroup vfs_ext2

    This function shuts down fs_ext2, basically undoing what fs_ext2_init() did.

    \retval 0           On success. No error conditions currently defined.
*/
int fs_ext2_shutdown(void);

/** \defgroup ext2_mount_flags          Mount Flags
    \brief                              Mount flags for fs_ext2
    \ingroup                            vfs_ext2

    These values are the valid flags that can be passed for the flags parameter
    to the fs_ext2_mount() function. Note that these can be combined, except for
    the read-only flag.

    Also, it is not possible to mount some filesystems as read-write. For
    instance, if the filesystem was marked as not cleanly unmounted (from Linux
    itself), the driver will fail to mount the device as read-write. Also, if
    the block device does not support writing, then the filesystem will not be
    mounted as read-write (for obvious reasons).

    These should stay synchronized with the ones in ext2fs.h.

    @{
*/
#define FS_EXT2_MOUNT_READONLY      0x00000000  /**< \brief Mount read-only */
#define FS_EXT2_MOUNT_READWRITE     0x00000001  /**< \brief Mount read-write */
/** @} */

/** \brief   Mount an ext2 filesystem in the VFS.
    \ingroup vfs_ext2

    This function mounts an ext2 filesystem to the specified mount point on the
    VFS. This function will detect whether or not an ext2 filesystem exists on
    the given block device and mount it only if there is actually an ext2
    filesystem.

    \param  mp          The path to mount the filesystem at.
    \param  dev         The block device containing the filesystem.
    \param  flags       Mount flags. Bitwise OR of values from ext2_mount_flags
    
    \retval 0           On success.
    \retval -1          On error.
*/
int fs_ext2_mount(const char *mp, kos_blockdev_t *dev, uint32_t flags);

/** \brief   Unmount an ext2 filesystem from the VFS.
    \ingroup vfs_ext2

    This function unmoutns an ext2 filesystem that was previously mounted by the
    fs_ext2_mount() function.

    \param  mp          The mount point of the filesystem to be unmounted.
    
    \retval 0           On success.
    \retval -1          On error.
*/
int fs_ext2_unmount(const char *mp);

/** \brief   Sync an ext2 filesystem, flushing all pending writes to the block
             device.
    \ingroup vfs_ext2

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
int fs_ext2_sync(const char *mp);

__END_DECLS
#endif /* !__EXT2_FS_EXT2_H */
