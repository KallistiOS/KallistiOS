/* KallistiOS ##version##

   newlib_times.c
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2022 Lawrence Sebald
   Copyright (C) 2023, 2024 Falco Girgis

*/

#include <errno.h>
#include <stdint.h>
#include <reent.h>
#include <sys/times.h>
#include <kos/timer.h>

clock_t _times_r(struct _reent *re, struct tms *tmsbuf) {
    (void)re;

    if(tmsbuf) {
        /*  User CPU Time: */
        const uint64_t precise_clock =
                  timer_us_gettime64();

        /* We have to protect against overflow. */
        if(precise_clock > UINT32_MAX) {
            errno = EOVERFLOW;
            return (clock_t)-1;
        }

        tmsbuf->tms_utime = precise_clock;

        /* System CPU Time: Unimplemented */
        tmsbuf->tms_stime = 0;
        /* Children User CPU Time: Unimplemented */
        tmsbuf->tms_cutime = 0;
        /* Children System CPU Time: Unimplemented */
        tmsbuf->tms_cstime = 0;

        return tmsbuf->tms_utime;
    }

    errno = EFAULT;
    return (clock_t)-1;
}
