/* KallistiOS ##version##

   dbglog.c
   Copyright (C) 2000,2001,2004 Megan Potter
   Copyright (C) 2026 Paul Cercueil
*/

#include <stdio.h>
#include <stdarg.h>

#include <kos/dbglog.h>

/* Default kernel debug log level: if a message has a level higher than this,
   it won't be shown. Set to DBG_DEAD to see basically nothing, and set to
   DBG_KDEBUG to see everything. DBG_INFO is generally a decent level. */
int dbglog_level = (DBGLOG_LEVEL_SUPPORT < DBG_INFO) ? DBGLOG_LEVEL_SUPPORT : DBG_INFO;

/* Set debug level */
void dbglog_set_level(int level) {
    dbglog_level = level;
}
