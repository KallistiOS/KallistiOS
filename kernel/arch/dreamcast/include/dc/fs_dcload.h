/* KallistiOS ##version##

   kernel/arch/dreamcast/include/dc/fs_dcload.h
   (c)2002 Andrew Kieschnick

*/

/** \file    dc/fs_dcload.h
    \brief   Implementation of dcload "filesystem".
    \ingroup vfs_dcload

    This file contains declarations related to using dcload, both in its -ip and
    -serial forms. This is only used for dcload-ip support if the internal
    network stack is not initialized at start via KOS_INIT_FLAGS().

    \author Andrew Kieschnick
    \see    dc/fs_dclsocket.h
*/

#ifndef __DC_FS_DCLOAD_H
#define __DC_FS_DCLOAD_H

/* Definitions for the "dcload" file system */

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdbool.h>

/** \defgroup vfs_dcload    PC
    \brief                  VFS driver for accessing a remote PC via
                            DC-Load/Tool
    \ingroup                vfs

    @{
*/

/* \cond */

/* Init func */
void fs_dcload_init_console(void);
void fs_dcload_init(void);
void fs_dcload_shutdown(void);

/* \endcond */

/** @} */

__END_DECLS

#endif  /* __DC_FS_DCLOAD_H */
