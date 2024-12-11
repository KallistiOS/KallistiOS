/* KallistiOS ##version##

   kos/fs_pty.h
   Copyright (C)2003 Megan Potter

*/

/** \file    kos/fs_pty.h
    \brief   Pseudo-terminal virtual file system.
    \ingroup vfs_pty

    This file system implements a pseudo-terminal like concept (similar to
    /dev/pty in Linux). A call to fs_pty_create() will create two file entries in
    the VFS, /pty/maXX and /pty/slXX (XX being some hexadecimal number). From
    there, anybody can open up either end and send data to the other side. Think
    of it as a simple message passing interface.

    This file system mounts on /pty.

    \author Megan Potter
*/

#ifndef __KOS_FS_PTY_H
#define __KOS_FS_PTY_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <kos/fs.h>

/** \defgroup vfs_pty   PTY
    \brief              VFS driver for accessing pseudo-TTY terminals
    \ingroup            vfs

    @{
*/

/** \brief  Create a new pseudo-terminal.

    This function creates a new pseudo-terminal, opening up two files in the
    /pty portion of the VFS.

    \param  buffer          Storage for the name of the PTY, apparently not
                            actually used (but potentially will be fixed at some
                            point). If it was implemented, the name of the PTY
                            would be here on successful return (if not NULL)
    \param  maxbuflen       The length of buffer
    \param  master_out      A pointer to store the file descriptor for the
                            master end in (must not be NULL)
    \param  slave_out       A pointer to store the file descriptor for the slave
                            end in (must not be NULL)
    \retval 0               On success
    \retval -1              On error

    \par    Error Conditions:
    \em     ENOMEM - out of memory
*/
int fs_pty_create(char * buffer, int maxbuflen, file_t * master_out, file_t * slave_out);

/** \cond */
void fs_pty_init(void);
void fs_pty_shutdown(void);
/** \endcond */

/** @} */

__END_DECLS

#endif  /* __KOS_FS_PTY_H */

