/* KallistiOS ##version##

   kos/fs_dev.h
   Copyright (C) 2024 Donald Haase

*/

/** \file    kos/fs_dev.h
    \brief   Container for /dev.
    \ingroup vfs_dev

    This is a thin filesystem that allows the /dev folder 
    and its contents to be read/listed as well new devices 
    to be added under it.

    \author Donald Haase
*/

#ifndef __DC_FS_DEV_H
#define __DC_FS_DEV_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <kos/fs.h>

/** \defgroup vfs_dev   Dev
    \brief              VFS driver for /dev
    \ingroup            vfs

    @{
*/

/* \cond */
/* Initialization */
void fs_dev_init(void);
void fs_dev_shutdown(void);
/* \endcond */

/** @} */

__END_DECLS

#endif  /* __DC_FS_DEV_H */

