/* KallistiOS ##version##

   kos/gcov.h
   Copyright (C) 2026 Andress Barajas

*/

/** \file    kos/gcov.h
    \brief   Code-coverage (.gcda) output on top of GCC's libgcov.
    \ingroup gcov

    Writes GCC coverage (.gcda) files on a KOS target. The coverage *runtime* is
    the toolchain's own libgcov; this addon supplies the file I/O that GCC's
    freestanding libgcov leaves to the platform. Build with --coverage and
    kos-cc wires everything up; coverage is flushed automatically at exit.

    \author Andress Barajas
*/

#ifndef __KOS_GCOV_H
#define __KOS_GCOV_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \defgroup gcov  GCOV
    \brief          GCOV coverage and PGO support for KOS
    \ingroup        debugging

    This file provides runtime support for GCOV, a coverage analysis tool built
    into GCC. GCOV allows you to determine which parts of your code were executed,
    how often, and which branches were taken. This is especially helpful for
    analyzing test coverage or identifying unused code paths.

    GCOV's runtime instrumentation writes `.gcda` files that serve two workflows
    here, both from a plain `--coverage` build:

      1. Code coverage -- which lines/branches ran, for gcov/lcov reports.
      2. Arc-based PGO -- recompile with `-fprofile-use` and the compiler
         optimizes from the edge counts (block layout, inlining, branch
         prediction, function reordering).

    What is NOT available is value-profiling PGO (`-fprofile-generate` /
    `-fprofile-values`). GCC's freestanding libgcov for these targets ships no
    value profilers, so those builds fail to link.

    Compile-time flag:

    - `--coverage`: shorthand for `-fprofile-arcs -ftest-coverage`. Instruments
      edges so per-function execution counts are written to `.gcda` at runtime.
      Also emits `.gcno` notes that map counters back to source lines and branches;
      required for human-readable coverage reports.

    When optimizing with `-fprofile-use` (step 4), these companion flags help:

    - `-fprofile-correction`: counter updates are non-atomic by default, so a
      multi-threaded run (KOS is threaded) can record an inconsistent profile;
      without this, `-fprofile-use` errors out. This smooths such
      inconsistencies with heuristics instead.

    - `-fprofile-partial-training`: by default `-fprofile-use` treats any
      function not seen during the run as cold and optimizes it for size. A
      single console run rarely exercises everything, so this tells the compiler
      to optimize un-exercised functions normally rather than pessimizing them.

    - `-fprofile-update=atomic`: (generate side) make counter updates atomic so a
      threaded run yields an exact, consistent profile, at some runtime cost --
      an alternative to `-fprofile-correction`.

    See the GCC manual's "Instrumentation Options" and "Optimize Options" for the
    authoritative descriptions of every flag above.

    Collecting, Analyzing, and Optimizing:

    1. **Compile with coverage support:**

        ```sh
        CFLAGS += --coverage
        ```

        This generates `.gcno` files alongside your object files.

    2. **Run your program on hardware:**

        Counts accumulate in memory as the program runs. Getting them out:

        - If your program returns from `main()` (or calls `exit()`), the data is
          flushed automatically -- this addon registers an `atexit()` handler --
          so you need not do anything.

        - If your program never exits (an infinite main loop, as many console
          programs have), that handler never runs. Call `kos_gcov_dump()`
          yourself to write the data.

        By default each `.gcda` is written at its full build-time path under
        `/pc` (Ex. /opt/toolchains/dc/kos/examples/dreamcast/profiling/gcov/gcov.gcda),
        so it matches the source it was generated from. Override the prefix from your
        program with `setenv(GCOV_PREFIX, "/your/path", 1)` before the data is dumped,
        and/or set `GCOV_PREFIX_STRIP` to drop leading path components.

    3. **Generate an HTML report with LCOV:**

        Use this Makefile target to capture and visualize results:

        ```make
        lcov:
          lcov \
            --gcov-tool=$(KOS_GCOV) \
            --directory . \
            --base-directory . \
            --capture \
            --rc geninfo_unexecuted_blocks=1 \
            --ignore-errors inconsistent,unused,source,empty \
            --output-file coverage.info
          genhtml coverage.info --output-directory html \
            --ignore-errors inconsistent,source
          open html/index.html
        ```

        The extra flags keep lcov 2.x (current Homebrew/distro versions, much
        stricter than 1.x) from aborting on coverage-data quirks that are normal
        for optimized and inlined/header code.

        This generates a full HTML report with annotated source code in the
        `html/` directory.

    4. **(Optional) Optimize with the profile (PGO):**

        Recompile the same sources with `-fprofile-use` plus the companion flags
        above (`-fprofile-correction` is required -- KOS is threaded):

        ```sh
        CFLAGS += -fprofile-use -fprofile-correction -fprofile-partial-training
        ```

        `-fprofile-use` reads each object's `.gcda` from the build tree (where
        step 2 wrote it) and optimizes from the edge counts.

    @{
*/

/** \brief Environment variable to set the output directory for `.gcda` files.
    \ingroup gcov

    If set, this value is prepended to the stripped source path to form the
    final output path.
*/
#define GCOV_PREFIX        "GCOV_PREFIX"

/** \brief Environment variable to control path stripping.
    \ingroup gcov

    Number of leading directory components removed from the build-time source
    path before GCOV_PREFIX is applied.
*/
#define GCOV_PREFIX_STRIP  "GCOV_PREFIX_STRIP"

/** \brief   Write coverage data for every instrumented object.

    Flushes the current in-memory counters to `.gcda` on disk (under GCOV_PREFIX
    / GCOV_PREFIX_STRIP).

    You normally do NOT need to call this: when a program returns from main()
    or calls exit(), this addon's atexit handler flushes the data for you. Call
    it explicitly only when that shutdown path never runs -- most commonly a
    program with an infinite main loop that never returns.

    \note Existing .gcda files are overwritten, not merged.
*/
void kos_gcov_dump(void);

/** @} */

__END_DECLS

#endif /* __KOS_GCOV_H */
