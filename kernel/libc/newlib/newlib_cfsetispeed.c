/* KallistiOS ##version##

   newlib_cfsetispeed.c
   Copyright (C) 2024 Andress Barajas

*/

#include <sys/termios.h>

int cfsetispeed(struct termios *termios_p, speed_t speed) {
    if(speed == 0)
        return -1;

    termios_p->c_ispeed = speed;
    return 0;
}
