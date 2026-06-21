/* KallistiOS ##version##

   lftpd_log.c
   Copyright (C) 2018 Jason von Nieda
*/

/* Leveled logging helpers (info / debug / error) for lftpd. */

#include "private/lftpd_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


void lftpd_log_internal(const char *level, const char *format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    int err = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if(err >= sizeof(buffer)) {
        return;
    }
    printf("%s %s\n", level, buffer);
}
