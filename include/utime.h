/* KallistiOS ##version##

   utime.h
   Copyright (C) 2024 Falco Girgis
*/

/** \file    utime.h
    \brief   KOS extension of Newlib's utime.h

    Newlib does not ever actually declare a prototype for utime() within
    its header, despite implementing it for SH. We add the prototype ourselves.

    \author Falco Girgis
*/

#ifndef __KOS_UTIME_H
#define __KOS_UTIME_H

__BEGIN_DECLS

#include <time.h>

struct utimbuf {
    time_t actime;  /**< access time */
    time_t modtime; /**< modification time */
};

extern int utime(const char *path, struct utimbuf *times);

__END_DECLS

#endif /* __KOS_UTIME_H */
