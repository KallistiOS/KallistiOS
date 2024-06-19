/* KallistiOS ##version##

   newlib_kill.c
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2024 Falco Girgis

*/

#include <kos/thread.h>
#include <kos/dbglog.h>

#include <arch/arch.h>

#include <sys/reent.h>

#include <signal.h>
#include <errno.h>
#include <stdlib.h>

/* This is the lowest-level entry point for any non-default, 
    non-blocked signals which have been raised. */
int _kill_r(struct _reent *reent, int pid, int sig) {
    /* We only support one process in KOS. */
    if(pid != KOS_PID) {
        reent->_errno = EINVAL;
        return -1;
    }

    switch(sig) {
        /* Cause abnormal/erroneous program termination,
           without calling atexit() handlers. */
        case SIGABRT:
            arch_abort();
            break;

        /* Exit with a generic failure code upon encountering
           uncaught exceptions. */
        case SIGSEGV:
        case SIGILL:
        case SIGFPE:
            exit(sig);
            break;

        /* Exit with a generic success code when the user manually
           or programmatically requests program exit. */
        case SIGTERM:
        case SIGINT:
            exit(sig);
            break;

        /* Warn if we're sent an unknown signal type. */
        default:
            dbglog(DBG_WARNING, "Received unknown signal type: %d\n", sig);
            break;
    }

    reent->_errno = EINVAL;
    return -1;
}
