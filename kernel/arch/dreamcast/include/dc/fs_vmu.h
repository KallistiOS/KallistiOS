/* KallistiOS ##version##

   dc/fs_vmu.h
   (c)2000-2001 Jordan DeLong

*/

/** \file    dc/fs_vmu.h
    \brief   VMU filesystem driver.
    \ingroup vfs_vmu

    The VMU filesystem driver mounts itself on /vmu of the VFS. Each memory card
    has its own subdirectory off of that directory (i.e, /vmu/a1 for slot 1 of
    the first controller). VMUs themselves have no subdirectories, so the driver
    itself is fairly simple.

    Files on a VMU must be multiples of 512 bytes in size, and should have a
    header attached so that they show up in the BIOS menu.

    This layer is built off of the vmufs layer, which does all the low-level
    operations. It is generally easier to work with things at this level though,
    so that you can use the normal libc file access functions.

    \author Megan Potter

    \see    dc/vmu_pkg.h
    \see    dc/vmufs.h
*/

#ifndef __DC_FS_VMU_H
#define __DC_FS_VMU_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/fs.h>

/** \defgroup vfs_vmu   VMU
    \brief              VFS driver for accessing Visual Memory Unit storage
    \ingroup            vfs

    @{
*/

/* \cond */
/* Initialization */
int fs_vmu_init(void);
int fs_vmu_shutdown(void);
/* \endcond */

/** @} */

__END_DECLS

#endif  /* __DC_FS_VMU_H */

