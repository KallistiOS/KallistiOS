/* KallistiOS ##version##

   kos/dbglog.h
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2026 Paul Cercueil

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

#include <kos/opts.h>
#include <stdio.h>

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
    \param  ...             Format string + Format arguments
    \see    dbglog_levels
*/
#define dbglog(lvl, ...) \
do { \
  if((lvl) <= DBGLOG_LEVEL_SUPPORT && (lvl) <= dbglog_level) { \
    __builtin_choose_expr((lvl) <= DBG_WARNING, \
                          fprintf(stderr, __VA_ARGS__), \
                          printf(__VA_ARGS__)); \
  } \
} while(0)

/** \defgroup   dbglog_levels   Log Levels
    \brief                      dbglog severity levels
    \ingroup                    logging

    This is the list of levels that are allowed to be passed into the dbglog()
    function, representing different levels of importance.

    For `DBG_SOURCE()` pass to it a define that controls specific debugging
    and if the define is defined, the logging will be outputted. If not defined
    the messages will only be outputted if the level is set to `DBG_MAX`.

    @{
*/
#define DBG_DISABLED    -1      /**< \brief No output allowed */
#define DBG_DEAD        0       /**< \brief The system is dead */
#define DBG_CRITICAL    1       /**< \brief A critical error message */
#define DBG_ERROR       2       /**< \brief A normal error message */
#define DBG_WARNING     3       /**< \brief Potential problem */
#define DBG_NOTICE      4       /**< \brief Normal but significant */
#define DBG_INFO        5       /**< \brief Informational messages */
#define DBG_DEBUG       6       /**< \brief User debug messages */
#define DBG_KDEBUG      7       /**< \brief Kernel debug messages */
#define DBG_MAX         8       /**< \brief All debug outputted */

#define DBG_SOURCE(x)   (__is_defined(x) ? DBG_DEAD : DBG_MAX) /**< \brief Verbose debugging of specific systems */
/** @} */

/** \brief   Set the debugging log level.
    \ingroup logging

    This function sets the level for which dbglog() will ignore messages for if
    the message has a higher level. This runtime setting does not override the
    `DBGLOG_LEVEL_SUPPORT` define.

    \param  level           The level to stop paying attention after.
    \see    dbglog_levels
*/
void dbglog_set_level(int level);

/** \cond */
extern int dbglog_level;
/** \endcond */

__END_DECLS

#endif  /* __KOS_DBGLOG_H */

