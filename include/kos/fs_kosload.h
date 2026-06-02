/* KallistiOS ##version##

   kos/fs_kosload.h
   Copyright (C) 2002 Andrew Kieschnick
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2025 Donald Haase
   Copyright (C) 2026 Andy Barajas

*/

/** \file    kos/fs_kosload.h
    \brief   kos-tool host filesystem VFS driver.
    \ingroup vfs_kosload

    Provides a /pc VFS mount backed by dc-load/dc-tool.
    \author Andrew Kieschnick
    \author Megan Potter
    \author Donald Haase
    \author Andy Barajas
*/

#ifndef __KOS_FS_KOSLOAD_H
#define __KOS_FS_KOSLOAD_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/kosload.h>
#include <kos/dbgio.h>

/** \addtogroup vfs_kosload
    @{
*/

/* \cond */
extern dbgio_handler_t dbgio_kosload;
/* \endcond */

#define KOSLOAD_TYPE_NONE   -1    /** \brief  No dc-load connection detected */
#define KOSLOAD_TYPE_SER    0     /** \brief  dc-load serial connection */
#define KOSLOAD_TYPE_IP     1     /** \brief  dc-load IP connection */

/** \brief  What type of kosload connection do we have? */
extern int kosload_type;

/* \cond */

/* Console (dbgio) setup */
void fs_kosload_init_console(void);

/* Init/shutdown of the /pc VFS mount. */
void fs_kosload_init(void);
void fs_kosload_shutdown(void);

/* \endcond */

/** @} */

__END_DECLS

#endif  /* __KOS_FS_KOSLOAD_H */
