/* KallistiOS ##version##

   newlib_tcgetattr.c
   Copyright (C) 2024 Andress Barajas

*/

#include <sys/ioctl.h>

int tcgetattr(int fd, struct termios *termios_p) {
    return ioctl(fd, TIOCGETA, termios_p);
}
