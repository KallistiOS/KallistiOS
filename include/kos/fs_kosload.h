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

    Provides console I/O and a /pc VFS mount backed by the kos-tool host tool
    (formerly dc-load/dc-tool).  Works on any platform that has a kos-tool
    bootstrap setting up the syscall vector; platform-specific addresses are
    resolved inside the implementation.

    \author Andrew Kieschnick
    \author Megan Potter
    \author Donald Haase
    \author Andy Barajas
*/

#ifndef __KOS_FS_KOSLOAD_H
#define __KOS_FS_KOSLOAD_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <kos/dbgio.h>

/** \defgroup vfs_kosload   PC
    \brief                  VFS driver for accessing a remote PC via kos-tool
    \ingroup                vfs

    @{
*/

/* \cond */
extern dbgio_handler_t dbgio_kosload;
/* \endcond */

/** \brief  Magic value written by the kos-tool bootstrap (same on all platforms) */
#define KOSLOADMAGICVALUE 0xdeadbeef

#define KOSLOAD_TYPE_NONE   -1    /** \brief  No kos-load connection detected */
#define KOSLOAD_TYPE_SER    0     /** \brief  kos-load serial connection */
#define KOSLOAD_TYPE_IP     1     /** \brief  kos-load IP connection */

/** \brief  What type of kosload connection do we have? */
extern int kosload_type;

/* \cond */

/** \brief  Stat structure used by the kos-tool wire protocol. */
typedef struct kosload_stat {
    unsigned short st_dev;
    unsigned short st_ino;
    int            st_mode;
    unsigned short st_nlink;
    unsigned short st_uid;
    unsigned short st_gid;
    unsigned short st_rdev;
    long           st_size;
    long           atime;
    long           st_spare1;
    long           mtime;
    long           st_spare2;
    long           ctime;
    long           st_spare3;
    long           st_blksize;
    long           st_blocks;
    long           st_spare4[2];
} kosload_stat_t;

/** \brief  Retrieve the host IP address and port from the kos-tool bootstrap.
    \param  ip      Receives the host IP address (network byte order).
    \param  port    Receives the host port number.
    \return The host IP address.
*/
uint32_t fs_kosload_gethostinfo(uint32_t *ip, uint32_t *port);

/** \brief  Send/receive a GDB remote-protocol packet over the kos-tool link.
    \param  in_buf      Data to send to the host (may be NULL).
    \param  in_size     Number of bytes to send.
    \param  out_buf     Buffer for the reply from the host (may be NULL).
    \param  out_size    Size of \p out_buf.
    \return Number of bytes received in \p out_buf.
*/
size_t fs_kosload_gdbpacket(const char *in_buf, size_t in_size,
                            char *out_buf, size_t out_size);

/* Tests for the kosload syscall being present. */
int syscall_kosload_detected(void);

/* Init func */
void fs_kosload_init_console(void);
void fs_kosload_init(void);
void fs_kosload_shutdown(void);

/* \endcond */

/** @} */

__END_DECLS

#endif  /* __KOS_FS_KOSLOAD_H */
