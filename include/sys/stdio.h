/** \file   sys/stdio.h
    \brief  Basic sys/stdio.h file from newlib.

    This file simply defines locking for files as no-ops if they aren't defined
    already.
*/

#ifndef _NEWLIB_STDIO_H
#define _NEWLIB_STDIO_H

#include <sys/cdefs.h>
__BEGIN_DECLS

// Cribbed from newlib sys/stdio.h

/* Internal locking macros, used to protect stdio functions.  In the
   general case, expand to nothing. */
#if !defined(_flockfile)
/** \brief  No-op lock on a file
    \param  fp              The file to lock.
*/
#  define _flockfile(fp)
#endif

#if !defined(_funlockfile)
/** \brief  No-op unlock a file
    \param  fp              The file to unlock.
*/
#  define _funlockfile(fp)
#endif

/* Keep compat with old dbglog.h inclusion */
#include <unistd.h>
#include <stdarg.h>
#include <kos/fs.h>

__END_DECLS

#endif /* _NEWLIB_STDIO_H */
