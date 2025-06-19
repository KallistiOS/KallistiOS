/* KallistiOS ##version##

   kos/fs_null.h
   Copyright (C) 2024 Donald Haase

*/

/** \file    kos/fs_null.h
    \brief   /dev/null, a black hole.
    \ingroup vfs_dev

    This is a IEEE Std 1003.1-2017 POSIX standard 
    'empty data source and infinite data sink'

    \author Donald Haase
*/

#ifndef __DC_FS_NULL_H
#define __DC_FS_NULL_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/fs.h>

/** \defgroup vfs_null   /dev/null
    \brief              VFS driver for /dev/null
    \ingroup            vfs

    @{
*/

/* \cond */
/* Initialization */
void fs_null_init(void);
void fs_null_shutdown(void);
/* \endcond */

/** @} */

__END_DECLS

#endif  /* __DC_FS_NULL_H */

