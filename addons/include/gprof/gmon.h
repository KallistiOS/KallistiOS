/* KallistiOS ##version##

   gprof/gmon.h
   Copyright (C) 2025 Andy Barajas

*/

#ifndef __GPROF_GMON_H
#define __GPROF_GMON_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <stdbool.h>

/** \defgroup debugging_gprof GPROF
    \brief    On-target gprof profiling support
    \ingroup  debugging

    This file provides utilities for profiling applications with gprof. Gprof
    lets you analyze where your program spent its time and which functions
    called which other functions during execution, helping you find
    performance bottlenecks.

    Profiling Steps:

    1. Build with profiling flags. \c -pg must be on both the compile and the
       link (as on a normal gprof system). KOS applies \c CFLAGS automatically
       when compiling, but the link command is hand-written, so add \c -pg to it
       directly:

        ```make
        CFLAGS += -pg -fno-omit-frame-pointer -fno-inline

        $(TARGET): $(OBJS)
            kos-cc -pg -o $(TARGET) $(OBJS)
        ```
        \c -fno-omit-frame-pointer is REQUIRED because GCC rejects \c -pg together
        with \c -fomit-frame-pointer (which KOS sets by default); \c -fno-inline
        keeps each function a distinct symbol in the report but is not required.

    2. Running your program to create gmon.out:

        Execute your program normally to completion. This writes the profiling
        data to \c /pc/gmon.out by default, or to \c $GMON_OUT_PREFIX.$pid if
        the \c GMON_OUT_PREFIX environment variable is set (see #GMON_OUT_PREFIX).

    3. Running gprof:

        ```sh
        $(KOS_GPROF) $(TARGET) gmon.out > gprof_output.txt
        ```
        Replace \c $(TARGET) with your .elf compiled program. This produces a
        human-readable report.

    4. Optional: visualize with gprof2dot + Graphviz:

        ```sh
        gprof2dot gprof_output.txt | dot -Tsvg -o graph.svg
        ```

    \author Andy Barajas

    @{
*/

/** \brief  Environment variable that redirects the gmon.out output path.

    If set, the profiling data is written to `<GMON_OUT_PREFIX>.<pid>` instead of
    the default location(`/pc/gmon.out`), letting you choose where it lands.
*/
#define GMON_OUT_PREFIX        "GMON_OUT_PREFIX"

/** \brief  Start gprof profiling for a specified address range.

    Allocates the profiling buffers, readies the per-architecture instrumentation,
    starts the histogram sampler, and begins profiling immediately. With \c -pg
    this is called automatically before \c main() (via the weak \c gprof_init()
    hook), so most programs never call it directly.

    Call it by hand only in a program not built with \c -pg, for a flat
    histogram-only profile -- without the \c -pg instrumentation there is no call
    graph.

    \param  lowpc   The lower bound of the address range to profile.
    \param  highpc  The upper bound of the address range to profile.
*/
void monstartup(uintptr_t lowpc, uintptr_t highpc);

/** \brief  Restart or stop gprof profiling.

    This function restarts or stops gprof profiling. It does NOT perform the
    initial setup (that is done by monstartup() before main()). Use it to pause
    profiling and then resume it later once you reach the section of code you
    want to profile.

    \param  enable  true to (re)start profiling, false to stop it.
*/
void moncontrol(bool enable);

/** \brief  Flush gmon.out and stop profiling.

    Writes the collected profile to its output file (see #GMON_OUT_PREFIX) and
    tears profiling down -- stopping the sampler and freeing its buffers. This
    runs automatically at program exit via atexit(), so most programs never call
    it.

    Call it explicitly if your program never returns from \c main() (e.g. a game
    loop) and you still want a gmon.out. It is idempotent, so a manual call plus
    the automatic exit-time one is harmless. Profiling stays off afterward until
    monstartup() is called again.
*/
void _mcleanup(void);

/** @} */

__END_DECLS

#endif  /* __GPROF_GMON_H */
