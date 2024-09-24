/* KallistiOS ##version##

   sys/ioctl.h
   Copyright (C) 2024 Andress Barajas

*/

/** \file    sys/uio.h
    \brief   Header for terminal control operations.
    \ingroup vfs_posix

    This file contains definitions for terminal control operations, as specified by
    the POSIX standard. It includes necessary constants and macros for performing
    various control operations on terminals using the ioctl system call.

    \author Andress Barajas
*/

#ifndef __SYS_IOCTL_H
#define __SYS_IOCTL_H

#include <sys/termios.h>

#include <sys/cdefs.h>

__BEGIN_DECLS

#include <kos/fs.h>

#ifndef TCGETS
#define TCGETS 0x5401
#endif

#ifndef TIOCGETA
#define TIOCGETA TCGETS
#endif

#ifndef IOCTL_ISO9660_GET_FIRST_EXTENT
#define IOCTL_ISO9660_GET_FIRST_EXTENT 0x5601
#endif

/* Define ioctl as an alias for fs_ioctl */
#define ioctl fs_ioctl

__END_DECLS

#endif /* __SYS_IOCTL_H */
