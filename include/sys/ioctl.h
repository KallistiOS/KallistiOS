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

#include <sys/termios.h>

#include <sys/cdefs.h>

__BEGIN_DECLS

#include <kos/fs.h>

/* Definition of struct winsize for terminal window size control */
struct winsize {
    unsigned short ws_row;     /* rows, in characters */
    unsigned short ws_col;     /* columns, in characters */
    unsigned short ws_xpixel;  /* horizontal size, pixels */
    unsigned short ws_ypixel;  /* vertical size, pixels */
};

/* Define ioctl as an alias for fs_ioctl */
#define ioctl fs_ioctl

#ifndef TCGETS
#define TCGETS 0x5401
#endif

#ifndef TIOCGETA
#define TIOCGETA TCGETS
#endif

#ifndef TIOCSETA
#define TIOCSETA  0x5402
#endif

#ifndef TIOCSETAW
#define TIOCSETAW 0x5403
#endif

#ifndef TIOCSETAF
#define TIOCSETAF 0x5404
#endif

/* Terminal window size */
#ifndef TIOCGWINSZ
#define TIOCGWINSZ 0x5413
#endif

/* Terminal control commands */
#ifndef TIOCMGET
#define TIOCMGET 0x5415
#endif

#ifndef TIOCMSET
#define TIOCMSET 0x5418
#endif

#ifndef TCSBRK
#define TCSBRK 0x741D
#endif

#ifndef TCSBRKP
#define TCSBRKP	 0x5425
#endif

/* Modem line control */
#ifndef TIOCM_RTS
#define TIOCM_RTS 0x004
#endif

#ifndef TIOCM_DTR
#define TIOCM_DTR 0x002
#endif

#ifndef TIOCM_CAR
#define TIOCM_CAR 0x040
#endif

#ifndef TIOCM_DSR
#define TIOCM_DSR 0x100
#endif

#ifndef TIOCM_RNG
#define TIOCM_RNG 0x080
#endif

#ifndef TIOCM_CTS
#define TIOCM_CTS 0x020
#endif

__END_DECLS

#endif /* __SYS_IOCTL_H */
