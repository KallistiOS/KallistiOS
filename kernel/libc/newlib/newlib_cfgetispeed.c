/* KallistiOS ##version##

   newlib_cfgetispeed.c
   Copyright (C) 2024 Andress Barajas

*/

#include <sys/termios.h>

speed_t cfgetispeed(const struct termios *termios_p) {
    return termios_p->c_ispeed;
}
