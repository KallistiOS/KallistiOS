/* KallistiOS ##version##

   kos/fs_romdisk.h
   (c)2001 Megan Potter

*/

/** \file    kos/fs_romdisk.h
    \brief   ROMFS virtual file system.
    \ingroup vfs_romdisk

    This file contains support for the romdisk VFS. This VFS allows you to make
    Linux-style ROMFS images and either embed them into your binary or load them
    at runtime from some other source (such as a CD-ROM). These images are made
    with the genromfs program that is included in the utils portion of the tree.

    You can choose to automount one ROMFS image by embedding it into your binary
    and using the appropriate KOS_INIT_FLAGS() setting. The embedded ROMFS will
    mount itself on /rd. You can also mount additional images that you load
    from some other source on whatever mountpoint you want.

    \author Megan Potter
*/

#ifndef __KOS_FS_ROMDISK_H
#define __KOS_FS_ROMDISK_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <kos/fs.h>

/** \defgroup vfs_romdisk   Romdisk
    \brief                  VFS driver for accessing romdisks binaries
    \ingroup                vfs

    @{
*/

/** \cond */
/* Initialize the file system */
void fs_romdisk_init(void);

/* De-init the file system; also unmounts any mounted images. */
void fs_romdisk_shutdown(void);
/** \endcond */

/* NOTE: the mount/unmount are _not_ thread safe as regards doing multiple
   mounts/unmounts in different threads at the same time, and they don't
   check for open files currently either. Caveat emptor! */

/** \brief  Mount a ROMFS image as a new filesystem.

    This function will mount a ROMFS image that has been loaded into memory to
    the specified mountpoint.

    \param  mountpoint      The directory to mount this romdisk on
    \param  img             The ROMFS image
    \param  own_buffer      If 0, you are still responsible for img, and must
                            free it if appropriate. If non-zero, img will be
                            freed when it is unmounted
    \retval 0               On success
    \retval -1              If fs_romdisk_init not called
    \retval -2              If img is invalid
    \retval -3              If a malloc fails
*/
int fs_romdisk_mount(const char * mountpoint, const uint8 *img, int own_buffer);

/** \brief  Unmount a ROMFS image.

    This function unmounts a ROMFS image that has been previously mounted with
    fs_romdisk_mount(). This function does not check for open files on the fs,
    so make sure that all files have been closed before calling it. If the VFS
    owns the buffer (own_buffer was non-zero when you called the mount function)
    then this function will also free the buffer.

    \param  mountpoint      The ROMFS to unmount
    \retval 0               On success
    \retval -1              On error

    \par    Error Conditions:
    \em     ENOENT - no such ROMFS was mounted
*/
int fs_romdisk_unmount(const char * mountpoint);

/** @} */

__END_DECLS

#endif  /* __KOS_FS_ROMDISK_H */

