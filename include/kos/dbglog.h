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

/** \defgroup   dbglog_levels   Log Levels
    \brief                      dbglog severity levels
    \ingroup                    logging

    This is the list of levels that are allowed to be passed into the dbglog()
    function, representing different levels of importance.

    @{
*/
#define DBG_DEAD        0       /**< \brief The system is dead */
#define DBG_CRITICAL    1       /**< \brief A critical error message */
#define DBG_ERROR       2       /**< \brief A normal error message */
#define DBG_WARNING     3       /**< \brief Potential problem */
#define DBG_NOTICE      4       /**< \brief Normal but significant */
#define DBG_INFO        5       /**< \brief Informational messages */
#define DBG_DEBUG       6       /**< \brief User debug messages */
#define DBG_KDEBUG      7       /**< \brief Kernel debug messages */
/** @} */

/** \brief   Set the debugging log level.
    \ingroup logging

    This function sets the level for which dbglog() will ignore messages for if
    the message has a higher level.

    \param  level           The level to stop paying attention after.
    \see    dbglog_levels
*/
void dbglog_set_level(int level);

__END_DECLS

#endif  /* __KOS_DBGLOG_H */

