/* KallistiOS ##version##

   kos/kosload.h
   Copyright (C) 2002 Andrew Kieschnick
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2025 Donald Haase
   Copyright (C) 2026 Andy Barajas

*/

/** \file    kos/kosload.h
    \brief   dc-tool host loader syscall driver.
    \ingroup vfs_kosload

    Low-level driver for the dc-load.  Provides the single multiplexed loader syscall
    and typed command wrappers around it, connection detection, and the
    GDB-over-loader transport.  The dc-tool host filesystem VFS and console (see
    <kos/fs_kosload.h>) are layered on top of this driver.

    The native syscall transport lives in each arch's own kosload_syscalls.c, and
    the only other platform-specific bit (the magic address used for detection)
    lives in the per-arch <arch/kosload.h>, so this driver is fully portable.

    \author Andrew Kieschnick
    \author Megan Potter
    \author Donald Haase
    \author Andy Barajas
*/

#ifndef __KOS_KOSLOAD_H
#define __KOS_KOSLOAD_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <dirent.h>

/** \defgroup vfs_kosload   PC
    \brief                  Driver and VFS for accessing a remote PC via dc-tool
    \ingroup                vfs

    @{
*/

/** \brief  Magic value at \ref KOSLOAD_MAGIC_ADDR when the dc-tool console is
            active: the loader is present and ready to service syscalls. */
#define KOSLOAD_CONSOLE_ON    0xdeadbeef

/** \brief  Magic value at \ref KOSLOAD_MAGIC_ADDR when the dc-tool console has
            been disabled: the loader is detached and syscalls must not be made. */
#define KOSLOAD_CONSOLE_OFF   0xfeedface

/** \brief  Commands multiplexed through the single loader syscall.

    The same command numbering is used by every host loader implementation,
    regardless of platform or transport.
*/
typedef enum {
    KOSLOAD_READ         = 0,
    KOSLOAD_WRITE        = 1,
    KOSLOAD_OPEN         = 2,
    KOSLOAD_CLOSE        = 3,
    KOSLOAD_CREAT        = 4,
    KOSLOAD_LINK         = 5,
    KOSLOAD_UNLINK       = 6,
    KOSLOAD_CHDIR        = 7,
    KOSLOAD_CHMOD        = 8,
    KOSLOAD_LSEEK        = 9,
    KOSLOAD_FSTAT        = 10,
    KOSLOAD_TIME         = 11,
    KOSLOAD_STAT         = 12,
    KOSLOAD_UTIME        = 13,
    KOSLOAD_ASSIGNWRKMEM = 14,
    KOSLOAD_EXIT         = 15,
    KOSLOAD_OPENDIR      = 16,
    KOSLOAD_CLOSEDIR     = 17,
    KOSLOAD_READDIR      = 18,
    KOSLOAD_GETHOSTINFO  = 19,
    KOSLOAD_GDBPACKET    = 20,
    KOSLOAD_REWINDDIR    = 21
} kosload_cmd_t;

/** \brief  Loader syscall transport function.

    A transport implements the multiplexed loader syscall for one particular
    link (the native bootstrap syscall, a network backend, etc).
*/
typedef int (*kosload_syscall_t)(kosload_cmd_t cmd, void *param1,
                                 void *param2, void *param3);

/** \brief  Select the transport used for all subsequent loader syscalls.
    \param  fn      The transport to install, or NULL to restore the native
                    bootstrap syscall.
*/
void kosload_syscall_set(kosload_syscall_t fn);

/** \brief  Bring up the network (kos-load-ip over KOS sockets) transport and
            install it as the active transport.
    \return 0 on success, <0 on failure.
*/
int kosload_syscall_net_init(void);

/** \brief  Tear down the network transport and restore the native one. */
void kosload_syscall_net_shutdown(void);

/* \cond */

/** \brief  Stat structure used by the dc-tool wire protocol. */
typedef struct kosload_stat {
    unsigned short st_dev;
    unsigned short st_ino;
    int st_mode;
    unsigned short st_nlink;
    unsigned short st_uid;
    unsigned short st_gid;
    unsigned short st_rdev;
    long st_size;
    long atime;
    long st_spare1;
    long mtime;
    long st_spare2;
    long ctime;
    long st_spare3;
    long st_blksize;
    long st_blocks;
    long st_spare4[2];
} kosload_stat_t;

/* Typed wrappers around the loader syscall, used by the dc-tool VFS layer. */
ssize_t kosload_read(uint32_t hnd, uint8_t *data, size_t len);
ssize_t kosload_write(uint32_t hnd, const uint8_t *data, size_t len);
int kosload_open(const char *fn, int oflags, int mode);
int kosload_close(uint32_t hnd);
int kosload_creat(const char *path, mode_t mode);
int kosload_link(const char *fn1, const char *fn2);
int kosload_unlink(const char *fn);
int kosload_chdir(const char *path);
int kosload_chmod(const char *path, mode_t mode);
off_t kosload_lseek(uint32_t hnd, off_t offset, int whence);
int kosload_fstat(int fildes, kosload_stat_t *buf);
time_t kosload_time(void);
int kosload_stat(const char *__RESTRICT path, kosload_stat_t *__RESTRICT buf);
/* int kosload_utime(const char *path, const struct utimbuf *times); */
int kosload_opendir(const char *fn);
int kosload_closedir(uint32_t hnd);
struct dirent *kosload_readdir(uint32_t hnd);
int kosload_rewinddir(uint32_t hnd);
int kosload_assignwrkmem(int *buf);
void kosload_exit(void);

/* \endcond */

/** \brief  Retrieve the host IP address and port from the dc-tool bootstrap.
    \param  ip      Receives the host IP address (network byte order).
    \param  port    Receives the host port number.
    \return The host IP address.
*/
uint32_t kosload_gethostinfo(uint32_t *ip, uint32_t *port);

/** \brief  Send/receive a GDB remote-protocol packet over the dc-tool link.
    \param  in_buf      Data to send to the host (may be NULL).
    \param  in_size     Number of bytes to send.
    \param  out_buf     Buffer for the reply from the host (may be NULL).
    \param  out_size    Size of \p out_buf.
    \return Number of bytes received in \p out_buf.
*/
size_t kosload_gdbpacket(const char *in_buf, size_t in_size,
                         char *out_buf, size_t out_size);

/* \cond */

/* Tests for the kosload syscall being present. */
int syscall_kosload_detected(void);

/* \endcond */

/** @} */

__END_DECLS

#endif  /* __KOS_KOSLOAD_H */
