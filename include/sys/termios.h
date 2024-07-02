/* KallistiOS ##version##

   sys/termios.h
   Copyright (C) 2024 Andress Barajas

*/

/** \file    sys/termios.h
    \brief   Header for terminal I/O control.
    \ingroup vfs_posix

    This file contains definitions for terminal I/O control operations, as specified by
    the POSIX standard. The termios structure and associated constants and functions
    are used to configure and control asynchronous communications ports.

    \author Andress Barajas
*/

#ifndef __SYS_TERMIOS_H
#define __SYS_TERMIOS_H

#include <sys/cdefs.h>

__BEGIN_DECLS

#include <stdint.h>

#define NCCS 32

typedef uint32_t tcflag_t;
typedef uint8_t cc_t;
typedef uint32_t speed_t;

struct termios {
    tcflag_t c_iflag;  /* input modes */
    tcflag_t c_oflag;  /* output modes */
    tcflag_t c_cflag;  /* control modes */
    tcflag_t c_lflag;  /* local modes */
    cc_t c_cc[NCCS];   /* control chars */
    speed_t c_ispeed;  /* input speed */
    speed_t c_ospeed;  /* output speed */
};

int tcgetattr(int fd, struct termios *termios_p);

__END_DECLS

#endif /* __SYS_TERMIOS_H */
