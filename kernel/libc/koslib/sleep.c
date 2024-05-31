/* KallistiOS ##version##

   sleep.c
   Copyright (C) 2005 Walter van Niftrik
   Copyright (C) 2024 Falco Girgis
*/

#include <kos/thread.h>

unsigned int sleep(unsigned int seconds) {
    thd_sleep_ms(seconds * 1000);

    return 0;
}
