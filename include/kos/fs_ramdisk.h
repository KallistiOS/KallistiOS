/* KallistiOS ##version##

   kos/fs_ramdisk.h
   (c)2002 Megan Potter

*/

/** \file    kos/fs_ramdisk.h
    \brief   RAM-based virtual file system.
    \ingroup vfs_ramdisk

    This file contains support for a ramdisk VFS. This VFS allows you to map
    memory into files that will appear in /ram. Files in this VFS can grow as
    large as memory allows, and there is full read/write support here. This is
    useful, for (for instance) caching files read from the CD-ROM or for making
    temporary files.

    You only have one ramdisk available, and its mounted on /ram.

    \author Megan Potter
*/

#ifndef __KOS_FS_RAMDISK_H
#define __KOS_FS_RAMDISK_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <kos/fs.h>

/** \defgroup vfs_ramdisk   Ramdisk
    \brief                  Filesystem driver for accessing in-ram images
    \ingroup                vfs

    @{
*/

/** \cond */
void fs_ramdisk_init(void);
void fs_ramdisk_shutdown(void);
/** \endcond */

/** \brief  Attach a block of memory as a file in the ramdisk.

    This function takes a block of memory and associates it with a file on the
    ramdisk. This memory should be allocated with malloc(), as an unlink() of
    the file will call free on the block of memory. The ramdisk then effectively
    takes control of the block, and is responsible for it at that point.

    \param  fn              The name to give the new file
    \param  obj             The block of memory to associate
    \param  size            The size of the block of memory
    \retval 0               On success
    \retval -1              On failure
*/
int fs_ramdisk_attach(const char * fn, void * obj, size_t size);

/** \brief  Detach a file from the ramdisk.

    This function retrieves the block of memory associated with the file,
    removing it from the ramdisk. You are responsible for freeing obj when you
    are done with it.

    \param  fn              The name of the file to look for.
    \param  obj             A pointer to return the address of the object in.
    \param  size            A pointer to return the size of the object in.
    \retval 0               On success
    \retval -1              On failure
*/
int fs_ramdisk_detach(const char * fn, void ** obj, size_t * size);

/** @} */

__END_DECLS

#endif  /* __KOS_FS_RAMDISK_H */

