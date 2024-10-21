/* KallistiOS ##version##

   newlib_cfgetospeed.c
   Copyright (C) 2024 Andress Barajas

*/

#include <sys/termios.h>

speed_t cfgetospeed(const struct termios *termios_p) {
    return termios_p->c_ospeed;
}
