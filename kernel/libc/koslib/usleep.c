/* KallistiOS ##version##

   usleep.c
   Copyright (C) 2001, 2004 Megan Potter
   Copyright (C) 2024 Falco Girgis
*/

#include <kos/thread.h>

/* usleep() */
void usleep(unsigned long usec) {
    thd_sleep_us(usec);
}

