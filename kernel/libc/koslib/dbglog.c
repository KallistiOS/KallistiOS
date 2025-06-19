/* KallistiOS ##version##

   dbglog.c
   Copyright (C)2000,2001,2004 Megan Potter
*/

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

#include <kos/dbglog.h>
#include <kos/thread.h>
#include <kos/dbgio.h>
#include <kos/fs.h>
#include <arch/spinlock.h>

/* Not re-entrant */
static char printf_buf[1024];
static spinlock_t mutex = SPINLOCK_INITIALIZER;

/* Default kernel debug log level: if a message has a level higher than this,
   it won't be shown. Set to DBG_DEAD to see basically nothing, and set to
   DBG_KDEBUG to see everything. DBG_INFO is generally a decent level. */
int dbglog_level = DBG_KDEBUG;

/* Set debug level */
void dbglog_set_level(int level) {
    dbglog_level = level;
}

/* Kernel debug logging facility */
void dbglog(int level, const char *fmt, ...) {
    va_list args;
    int i;

    /* If this log level is blocked out, don't even bother */
    if(level > dbglog_level)
        return;

    /* We only try to lock if the message isn't urgent */
    if(level >= DBG_ERROR && !irq_inside_int())
        spinlock_lock(&mutex);

    va_start(args, fmt);
    i = vsnprintf(printf_buf, sizeof(printf_buf), fmt, args);
    va_end(args);

    if(i > 0) {
        if(irq_inside_int() || 
            (fs_write(STDOUT_FILENO, printf_buf, strlen(printf_buf)) < 0))
                dbgio_write_str(printf_buf);        
    }

    if(level >= DBG_ERROR && !irq_inside_int())
        spinlock_unlock(&mutex);
}
