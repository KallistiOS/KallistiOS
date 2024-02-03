/* KallistiOS ##version##

   dbglog.c
   Copyright (C) 2000, 2001, 2004 Megan Potter
   Copyright (C) 2023 Falco Girgis 
*/

#include <stdio.h>
#include <kos/dbglog.h>
#include <stdarg.h>
#include <string.h>
#include <kos/thread.h>
#include <kos/dbgio.h>

/* Default kernel debug log level: if a message has a level higher than this,
   it won't be shown. Set to DBG_DEAD to see basically nothing, and set to
   DBG_KDEBUG to see everything. DBG_INFO is generally a decent level. */
static int dbglog_level = DBG_KDEBUG;

/* Set debug level */
void dbglog_set_level(int level) {
    dbglog_level = level;
}

/* Get debug level */
int dbglog_get_level(void) {
    return dbglog_level;
}

void vdbglog(int level, const char *fmt, va_list *var_args) {
    if(level > dbglog_level) 
        return;

    dbgio_vprintf(fmt, var_args);
}

/* Kernel debug logging facility */
void dbglog(int level, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vdbglog(level, fmt, &args);
    va_end(args);
}
