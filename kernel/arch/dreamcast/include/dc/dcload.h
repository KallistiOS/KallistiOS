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

size_t dcload_gdbpacket(const char* in_buf, size_t in_size, char* out_buf, size_t out_size);
/** @} */

__END_DECLS

#endif /* __DC_DCLOAD_H */
