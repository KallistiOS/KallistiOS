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
    and using the appropriate flags (INIT_DEFAULT by itself or INIT_FS_ROMDISK with other flags)
    when calling the KOS_INIT_FLAGS() macro with a custom flag selection. The embedded ROMFS
    will mount itself on /rd.

    \warning
    An embedded romdisk image is linked to your executable and cannot be evicted from
    system RAM!
    
    Mounting additional images that you load from some other sources (such as a modified BIOS)
    on whatever mountpoint you want, is also possible. Using fs_romdisk_mount() and passing a
    pointer to the location of a romdisk image will mount it.

    \remark 
    Mounted images will reside in system RAM for as long as your program is running
    or until you unmount them with fs_romdisk_unmount(). The size of your generated
    ROMFS image must be kept below 16MB, with 14MB being the maximum recommended size, 
    as your binary will also reside in RAM and you need to leave some memory available
    for it. Generating files larger than the available RAM will lead to system crashes.
    
    A romdisk filesystem image can be created by adding "KOS_ROMDISK_DIR=" to your Makefile
    and pointing it to the directory contaning all the resources you wish to have embeded in
    filesystem image. A rule to create the image is provided in the rules provided in Makefile.rules,
    the created object file must be linked with your binary file by adding romdisk.o to your 
    list of objects.
    
    \see INIT_FS_ROMDISK
    \see KOS_INIT_FLAGS()

    \author Megan Potter
*/

#ifndef __KOS_FS_ROMDISK_H
#define __KOS_FS_ROMDISK_H

#include <sys/cdefs.h>
#include <stdint.h>
__BEGIN_DECLS

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
int fs_romdisk_mount(const char * mountpoint, const uint8_t *img, int own_buffer);

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

