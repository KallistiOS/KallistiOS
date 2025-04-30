/* KallistiOS ##version##

   sys/ioctl.h
   Copyright (C) 2024 Andress Barajas

*/

/** \file    sys/ioctl.h
    \brief   Header for terminal control operations.
    \ingroup vfs_posix

    This file contains definitions for terminal control operations, as specified by
    the POSIX standard. It includes necessary constants and macros for performing
    various control operations on terminals using the ioctl system call.

    \author Andress Barajas
*/

#ifndef __SYS_IOCTL_H
#define __SYS_IOCTL_H

#include <sys/cdefs.h>

__BEGIN_DECLS

#include <kos/fs.h>

/* Define ioctl as an alias for fs_ioctl */
#define ioctl fs_ioctl

__END_DECLS

#endif /* __SYS_IOCTL_H */
