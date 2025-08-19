/* KallistiOS ##version##

   arch/dreamcast/include/dc/perf_monitor.h
 * Copyright (C) 2024 Paul Cercueil

*/

/** \file    dc/perf_monitor.h
    \brief   Low-level performance monitor
    \ingroup perf_monitor

    This file contains an API that can be used to monitor specific
    performance events in one or several functional blocks.

    \author Paul Cercueil
*/

#ifndef __KOS_PERF_MONITOR_H
#define __KOS_PERF_MONITOR_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <dc/perfctr.h>
#include <stdint.h>
#include <stdio.h>

/** \defgroup   perf_monitor Performance monitor
    \brief      Code performance monitor
    \ingroup    debugging

    The performance monitor API is built on top of the performance counter API,
    and as such cannot be used at the same time.
    With this API, programs can set probe points in different functional blocks
    and later obtain statistics about the execution of said functional blocks.

    @{
*/

/** /cond */
struct perf_monitor {
    const char *fn;
    unsigned int line;
    uint64_t calls;
    uint64_t time_ns, time_start;
    uint64_t event0, event0_start;
    uint64_t event1, event1_start;
};

void __stop_perf_monitor(struct perf_monitor **monitor);

struct perf_monitor *__start_perf_monitor(struct perf_monitor *monitor);

#define __perf_monitor(f, l) \
    static struct perf_monitor __perf_monitor_##l \
        __attribute__((section(".monitors"))) = { f, l, }; \
    struct perf_monitor *___perf_monitor_##l \
        __attribute__((cleanup(__stop_perf_monitor))) = \
        __start_perf_monitor(&__perf_monitor_##l)

#define _perf_monitor(f, l) __perf_monitor(f, l)

#define __perf_monitor_if(f, l, tst) ({ \
    static struct perf_monitor __perf_monitor_##l \
        __attribute__((section(".monitors"))) = { f, l, }; \
    __perf_monitor_##l.calls++; \
    (tst) ? (__perf_monitor_##l.event1++,1) : (__perf_monitor_##l.event0++,0); \
})

#define _perf_monitor_if(f, l, tst) __perf_monitor_if(f, l, tst)
/** /endcond */

/** \brief  Register a performance monitor in the current functional block

    The performance monitor will run from the moment this macro is used, till
    the end of the functional block.
*/
#define perf_monitor() _perf_monitor(__func__, __LINE__)

/** \brief  Register a performance monitor for branch likeliness analysis

    This macro is designed to be used inside an "if" expression, for instance:
    if(perf_monitor_if(!strcmp("test", str))) { ... }

    The resulting performance monitor will measure the number of calls, and
    the number of times the branch was taken (in event1) and the number of
    time it was not (in event0).

    \param  tst             The boolean expression that is normally used inside
                            the "if" check
*/
#define perf_monitor_if(tst) _perf_monitor_if(__func__, __LINE__, tst)

/** \brief  Initialize the performance monitor system

    Set up the performance monitor system. Note that using the performance
    monitor system will conflict with any external usage of the performance
    counter API.

    \param  event1          The first event mode (pef_cntr_event_t).
    \param  event2          The second event mode (pef_cntr_event_t).
*/
void perf_monitor_init(perf_cntr_event_t event1, perf_cntr_event_t event2);

/** \brief  De-initialize the performance monitor system

    After this function is called, the performance counter API can be
    used again.
*/
void perf_monitor_exit(void);

/** \brief  Print statistics about the probe points to the given file descriptor
    \param  f               A valid file descriptor to which the messages will
                            be printed. Use "stdout" for the standard output.
*/
void perf_monitor_print(FILE *f);

/** @} */

__END_DECLS

#endif /* __KOS_PERF_MONITOR_H */
