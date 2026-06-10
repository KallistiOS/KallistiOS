/* KallistiOS ##version##

   kernel/arch/dreamcast/include/dc/fs_dcload.h
   Copyright (C) 2002 Andrew Kieschnick
   Copyright (C) 2026 Andy Barajas

*/

/** \file    dc/fs_dcload.h
    \brief   Deprecated compatibility shim for the old dcload API.
    \ingroup vfs_kosload

    \deprecated
    The dc-load "filesystem" has been made platform-agnostic and renamed to
    kos-load.  This header only provides backwards-compatible aliases for the
    old dc-load names.

    \author Andrew Kieschnick
    \see    kos/fs_kosload.h
    \see    kos/kosload.h
*/

#ifndef __DC_FS_DCLOAD_H
#define __DC_FS_DCLOAD_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <arch/kosload.h>
#include <kos/fs_kosload.h>

/** \addtogroup vfs_kosload
    @{
*/

/** \brief  \deprecated Use \ref dbgio_kosload. */
#define dbgio_dcload        dbgio_kosload

/** \brief  \deprecated Use \ref KOSLOAD_CONSOLE_ON. */
#define DCLOADMAGICVALUE    KOSLOAD_CONSOLE_ON

/** \brief  \deprecated Use \ref KOSLOAD_MAGIC_ADDR. */
#define DCLOADMAGICADDR     ((unsigned int *)KOSLOAD_MAGIC_ADDR)

/** \brief  \deprecated Use \ref KOSLOAD_TYPE_NONE. */
#define DCLOAD_TYPE_NONE    KOSLOAD_TYPE_NONE
/** \brief  \deprecated Use \ref KOSLOAD_TYPE_SER. */
#define DCLOAD_TYPE_SER     KOSLOAD_TYPE_SER
/** \brief  \deprecated Use \ref KOSLOAD_TYPE_IP. */
#define DCLOAD_TYPE_IP      KOSLOAD_TYPE_IP

/** \brief  \deprecated Use \ref kosload_type. */
#define dcload_type         kosload_type

/* \cond */

/** \brief  \deprecated Use syscall_kosload_detected(). */
__depr("use syscall_kosload_detected()")
static inline int fs_dcload_detected(void) {
    return syscall_kosload_detected();
}

/* \endcond */

/** @} */

__END_DECLS

#endif  /* __DC_FS_DCLOAD_H */
