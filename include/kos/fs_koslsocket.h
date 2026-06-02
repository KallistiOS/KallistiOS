/* KallistiOS ##version##

   kos/fs_koslsocket.h
   Copyright (C) 2007, 2008 Lawrence Sebald

*/

/** \file    kos/fs_koslsocket.h
    \brief   Implementation of kos-load IP mode over KOS sockets.
    \ingroup vfs_kosload

    This file contains declarations related to using kos-load over the KOS
    sockets system. If kos-load IP support is enabled at the same time as
    network support, this is how the communications will happen. There isn't
    really anything that users will need to deal with in here.

    \author Lawrence Sebald
*/

#ifndef __KOS_FS_KOSLSOCKET_H
#define __KOS_FS_KOSLSOCKET_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <kos/fs_kosload.h>

/** \addtogroup vfs_kosload
   @{
*/

/* \cond */
extern dbgio_handler_t dbgio_kosl;

/* Initialization */
void fs_koslsocket_init_console(void);
int fs_koslsocket_init(void);

void fs_koslsocket_shutdown(void);
/* \endcond */

/** @} */

__END_DECLS

#endif /* __KOS_FS_KOSLSOCKET_H */
