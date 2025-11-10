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
#include <kos/fs.h>
#include <kos/dbgio.h>

/** \defgroup vfs_dcload    PC
    \brief                  VFS driver for accessing a remote PC via
                            DC-Load/Tool
    \ingroup                vfs

    @{
*/

/* \cond */
extern dbgio_handler_t dbgio_dcload;
/* \endcond */

/* dcload magic value */
/** \brief  The dcload magic value! */
#define DCLOADMAGICVALUE 0xdeadbeef

/** \brief  The address of the dcload magic value */
#define DCLOADMAGICADDR (unsigned int *)0x8c004004

/* Are we using dc-load-serial or dc-load-ip? */
#define DCLOAD_TYPE_NONE    -1      /**< \brief No dcload connection */
#define DCLOAD_TYPE_SER     0       /**< \brief dcload-serial connection */
#define DCLOAD_TYPE_IP      1       /**< \brief dcload-ip connection */

/** \brief  What type of dcload connection do we have? */
extern int dcload_type;

/* \cond */

/* Tests for the dcload syscall being present. */
int syscall_dcload_detected(void);

/* Init func */
void fs_dcload_init_console(void);
void fs_dcload_init(void);
void fs_dcload_shutdown(void);

/* \endcond */

/** @} */

__END_DECLS

#endif  /* __DC_FS_DCLOAD_H */
