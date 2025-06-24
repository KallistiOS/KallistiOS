/* KallistiOS ##version##

   dc/dcload.h
   Copyright (C) 2025 Donald Haase
*/

/** \file      dc/dcload.h
    \brief     Functions to access the system calls provided by dcload.
    \ingroup   dcload_syscalls

    \author Donald Haase
*/

/** \defgroup  dcload_syscalls dcload system calls
    \brief     API for dcload's system calls
    \ingroup   system_calls

    This module encapsulates all the commands provided by dcload
    via its syscall function.

    @{
*/

#ifndef __DC_DCLOAD_H
#define __DC_DCLOAD_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

typedef enum {
    DCLOAD_READ         = 0,
    DCLOAD_WRITE        = 1,
    DCLOAD_OPEN         = 2,
    DCLOAD_CLOSE        = 3,
    DCLOAD_CREAT        = 4,
    DCLOAD_LINK         = 5,
    DCLOAD_UNLINK       = 6,
    DCLOAD_CHDIR        = 7,
    DCLOAD_CHMOD        = 8,
    DCLOAD_LSEEK        = 9,
    DCLOAD_FSTAT        = 10,
    DCLOAD_TIME         = 11,
    DCLOAD_STAT         = 12,
    DCLOAD_UTIME        = 13,
    DCLOAD_ASSIGNWRKMEM = 14,
    DCLOAD_EXIT         = 15,
    DCLOAD_OPENDIR      = 16,
    DCLOAD_CLOSEDIR     = 17,
    DCLOAD_READDIR      = 18,
    DCLOAD_GETHOSTINFO  = 19,
    DCLOAD_GDBPACKET    = 20,
    DCLOAD_REWINDDIR    = 21
} dcload_cmd_t;

int syscall_dcload(dcload_cmd_t cmd, void *param1, void *param2, void *param3);
ssize_t dcload_read(uint32_t hnd, uint8_t *data, size_t len);
ssize_t dcload_write(uint32_t hnd, const uint8_t *data, size_t len);
int dcload_open(const char *fn, int oflags, int mode);
int dcload_close(uint32_t hnd);
/* CREAT */
int dcload_link(const char *fn1, const char *fn2);
int dcload_unlink(const char *fn);
/* CHDIR */
/* CHMOD */
off_t dcload_lseek(uint32_t hnd, off_t offset, int whence);
/* FSTAT */
/* TIME */

/* dcload stat */
typedef struct dcload_stat {
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
} dcload_stat_t;

int dcload_stat(const char *restrict path, dcload_stat_t *restrict buf);
/* UTIME */

/* EXIT */
int dcload_opendir(const char *fn);
int dcload_closedir(uint32_t hnd);

/* dcload dirent */
typedef struct dcload_dirent {
    long            d_ino;  /* inode number */
    off_t           d_off;  /* offset to the next dirent */
    unsigned short  d_reclen;/* length of this record */
    unsigned char   d_type;         /* type of file */
    char            d_name[256];    /* filename */
} dcload_dirent_t;

dcload_dirent_t *dcload_readdir(uint32_t hnd);
uint32_t dcload_gethostinfo(uint32_t *ip, uint32_t *port);
size_t dcload_gdbpacket(const char* in_buf, size_t in_size, char* out_buf, size_t out_size);
int dcload_rewinddir(uint32_t hnd);
/** @} */

__END_DECLS

#endif /* __DC_DCLOAD_H */
