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
#include <stdint.h>

__BEGIN_DECLS

typedef uint32_t tcflag_t;
typedef uint8_t cc_t;
typedef uint32_t speed_t;

#define NCCS 32
struct termios {
    tcflag_t c_iflag;  /* Input modes */
    tcflag_t c_oflag;  /* Output modes */
    tcflag_t c_cflag;  /* Control modes */
    tcflag_t c_lflag;  /* Local modes */
    cc_t c_cc[NCCS];   /* Control chars */
    speed_t c_ispeed;  /* Input speed */
    speed_t c_ospeed;  /* Output speed */
};

/* Control characters */
#define VTIME   5
#define VMIN    6
#define VSTART  8
#define VSTOP   9

/* Input modes */
#define BRKINT  0x00000002
#define INPCK   0x00000020
#define ISTRIP  0x00000040
#define ICRNL   0x00000400
#define IXON    0x00002000
#define IXOFF   0x00010000

/* Control modes */
#define CS5     0x00000000
#define CS6     0x00000020
#define CS7     0x00000040
#define CS8     0x00000060
#define CSTOPB  0x00000100
#define PARENB  0x00000400
#define PARODD  0x00001000
#define CSIZE   0x00000060
#define CRTSCTS 0x80000000

/* Local modes */
#define ECHO    0x00000010
#define ECHOE   0x00000020
#define ECHOK   0x00000040
#define ECHONL  0x00000100
#define ICANON  0x00000002
#define IEXTEN  0x00100000
#define ISIG    0x00000001
#define NOFLSH  0x00000200
#define TOSTOP  0x00000400

/* Output modes */
#define OPOST   0x00000001

/* Optional actions for tcsetattr */
#define TCSANOW    0
#define TCSADRAIN  1
#define TCSAFLUSH  2

/* Baud rates - not applicable in all cases but added for completeness */
#define B0      0
#define B50     50
#define B75     75
#define B110    110
#define B134    134
#define B150    150
#define B200    200
#define B300    300
#define B600    600
#define B1200   1200
#define B1800   1800
#define B2400   2400
#define B4800   4800
#define B9600   9600
#define B19200  19200
#define B38400  38400
#define B57600  57600
#define B115200 115200
#define B230400 230400

int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
int tcsendbreak(int fd, int duration);

int cfsetispeed(struct termios *termios_p, speed_t speed);
int cfsetospeed(struct termios *termios_p, speed_t speed);
speed_t cfgetispeed(const struct termios *termios_p);
speed_t cfgetospeed(const struct termios *termios_p);

__END_DECLS

#endif /* __SYS_TERMIOS_H */
