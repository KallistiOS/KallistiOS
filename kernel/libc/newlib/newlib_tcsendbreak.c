/* KallistiOS ##version##

   newlib_tcsendbreak.c
   Copyright (C) 2024 Andress Barajas

*/
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

int tcsendbreak(int fd, int duration) {
    /* Check if the file descriptor is valid and associated with a terminal */
    if(!isatty(fd)) {
        errno = ENOTTY;
        return -1;
    }

    if(duration < 0) {
        errno = EINVAL;
        return -1;
    }
    
    if(duration == 0) {
        struct timespec ts = {0, 250000000}; /* 0.25 seconds */
        if(ioctl(fd, TCSBRK, 0) < 0)
            return -1;
        
        nanosleep(&ts, NULL); /* Sleep for 0.25 seconds */
    } else {
        if(ioctl(fd, TCSBRKP, duration) < 0) {
            return -1;
        }
    }

    return 0;
}
