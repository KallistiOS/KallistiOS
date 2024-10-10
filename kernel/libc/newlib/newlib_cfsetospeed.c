/* KallistiOS ##version##

   newlib_cfsetospeed.c
   Copyright (C) 2024 Andress Barajas

*/

#include <sys/termios.h>

int cfsetospeed(struct termios *termios_p, speed_t speed) {
    if(speed == 0)
        return -1;
    
    termios_p->c_ospeed = speed;
    return 0;
}
