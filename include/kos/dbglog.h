/* KallistiOS ##version##

   kos/dbglog.h
   Copyright (C)2004 Megan Potter

*/

/** \file    kos/dbglog.h
    \brief   A debugging log.
    \ingroup logging

    This file contains declarations related a debugging log. This log can be
    used to restrict log messages, for instance to make it so that only the most
    urgent of messages get printed for a release version of a program.

    \author Megan Potter
*/

#ifndef __KOS_DBGLOG_H
#define __KOS_DBGLOG_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <unistd.h>
#include <stdarg.h>
#include <kos/fs.h>
#include <kos/opts.h>

/** \defgroup logging   Logging
    \brief              KOS's Logging API 
    \ingroup            debugging
*/

/** \brief   Kernel debugging printf.
    \ingroup logging    

    This function is similar to printf(), but filters its output through a log
    level check before being printed. This way, you can set the level of debug
    info you want to see (or want your users to see).

    \param  level           The level of importance of this message.
    \param  fmt             Message format string.
    \param  ...             Format arguments
    \see    dbglog_levels
*/

void dbglog(int level, const char *fmt, ...) __printflike(2, 3);

/** \brief   Set the debugging log level.
    \ingroup logging

    This function sets the level for which dbglog() will ignore messages for if
    the message has a higher level. Only available with DBG_DYNLOG set.

    \param  level           The level to stop paying attention after.
    \see    dbglog_levels
*/
void dbglog_set_level(int level);

__END_DECLS

#endif  /* __KOS_DBGLOG_H */


