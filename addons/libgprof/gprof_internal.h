/* KallistiOS ##version##

   gprof_internal.h
   Copyright (C) 2025 Andy Barajas

   Internal interface between the portable gprof core (gmon.c) and the
   per-architecture backends under arch/<arch>/. Not part of the public API.

*/

#ifndef __GPROF_INTERNAL_H
#define __GPROF_INTERNAL_H

#include <stdint.h>

/** \brief  Record a call-graph arc.

    Both architecture backends funnel function-entry events into this single
    portable routine, which maintains the binary-search-tree call graph. It is
    a no-op unless profiling is currently enabled.

    \param  frompc  Address of the call site (the caller).
    \param  selfpc  Address of the function being entered (the callee).
*/
void gprof_mcount(uintptr_t frompc, uintptr_t selfpc);

/** \brief  Arm the architecture's function-entry instrumentation.

    Called by monstartup() once the profiling buffers are ready. The portable
    core provides a weak no-op default; an arch backend overrides it when it has
    something to set up.
*/
void gprof_arch_start(void);

/** \brief  Disarm the architecture's function-entry instrumentation.

    Called during cleanup. Weak no-op by default; overridden by backends that
    armed something in gprof_arch_start().
*/
void gprof_arch_stop(void);

#endif  /* __GPROF_INTERNAL_H */
