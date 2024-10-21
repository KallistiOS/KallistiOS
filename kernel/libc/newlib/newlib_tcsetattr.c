/* KallistiOS ##version##

   newlib_tcsetattr.c
   Copyright (C) 2024 Andress Barajas

*/

#include <sys/ioctl.h>
#include <errno.h>

int tcsetattr(int fd, int optional_actions, const struct termios *termios_p) {
    int cmd;
    
    switch(optional_actions) {
        case TCSANOW:
            cmd = TIOCSETA;
            break;
        case TCSADRAIN:
            cmd = TIOCSETAW;
            break;
        case TCSAFLUSH:
            cmd = TIOCSETAF;
            break;
        default:
            errno = EINVAL;
            return -1;
    }

    if(ioctl(fd, cmd, termios_p) == -1) {
        return -1;
    }

    return 0;
}
