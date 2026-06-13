/* KallistiOS ##version##

   gprof.c
   Copyright (C) 2025 Andy Barajas

   Exercises the libgprof runtime end to end: histogram time sampling and the
   call-graph recorder, producing a standard gmon.out for host-side gprof.

   The workload is built from a few out-of-line functions with a clear call
   hierarchy, so both halves of the profile are interesting:

     - the flat profile (where time is spent) is dominated by spin(), reached
       far more often through hot_path() than warm_path();
     - the call graph shows main -> workload -> {hot_path, warm_path, fibonacci}
       -> spin(), with spin()'s time split across its two callers, plus
       fibonacci()'s recursive self-edge.

   Nothing here calls into the runtime directly: linking with -pg auto-starts
   profiling before main() (via the weak gprof_init() hook) and flushes gmon.out
   on exit. Build/run/report steps are in README.md.

   A couple of details keep the optimizer from spoiling the demo:

     - the helpers are __attribute__((noinline, noclone)) so each stays a
       distinct symbol in the report (the Makefile also passes -fno-inline, and
       -fno-ipa-icf so hot_path/warm_path -- which are identical -- aren't folded
       into one symbol);
     - spin() takes its iteration count from a volatile, so -O2 cannot prove the
       call is loop-invariant and hoist it out of the loop -- otherwise spin()
       would run once per caller instead of hundreds of times.

   Results feed a volatile sink so nothing is dead-code eliminated.
*/

#include <stdio.h>

/* Workload sizing -- bump these for a longer run (more histogram samples) or
   trim them for a quicker one. The defaults run for a few seconds, plenty for
   the 10ms sampler. */
#define ROUNDS       8     /* times the whole workload repeats              */
#define HOT_ROUNDS   300   /* spin() calls per hot_path()  (the hot caller) */
#define WARM_ROUNDS  60    /* spin() calls per warm_path() (the cool caller)*/
#define SPIN_WORK    4000  /* FP iterations inside each spin() call         */
#define FIB_N        19    /* recursion depth: a self-edge with modest time */

static volatile double sink;

/* Read through a volatile so the optimizer can't treat spin(spin_iters) as a
   loop-invariant call and hoist it out of hot_path()/warm_path(). */
static volatile int spin_iters = SPIN_WORK;

/* Pure CPU burner: a fixed chunk of (software-emulated, hence expensive) double
   math. This is where the flat profile should show most of the time. */
static double __attribute__((noinline, noclone)) spin(int iters) {
    double x = 1.0;
    int i;

    for(i = 1; i <= iters; i++)
        x += (x * 1.0000001) / (double)i - 1e-7 * (double)i;

    return x;
}

/* The hot path: many spin() calls, so most of spin()'s time is attributed here
   in the call graph. */
static double __attribute__((noinline, noclone)) hot_path(int rounds) {
    double acc = 0.0;
    int i;

    for(i = 0; i < rounds; i++)
        acc += spin(spin_iters);

    return acc;
}

/* The warm path: the same work per call, but far fewer calls. */
static double __attribute__((noinline, noclone)) warm_path(int rounds) {
    double acc = 0.0;
    int i;

    for(i = 0; i < rounds; i++)
        acc += spin(spin_iters);

    return acc;
}

/* Naive recursion -- gives the call graph a self-edge. Kept shallow so its
   per-call instrumentation overhead does not dominate the profile. */
static long __attribute__((noinline, noclone)) fibonacci(int n) {
    if(n < 2)
        return n;

    return fibonacci(n - 1) + fibonacci(n - 2);
}

/* One unit of work, touching each path so the call graph has structure. */
static void __attribute__((noinline, noclone)) workload(void) {
    sink += hot_path(HOT_ROUNDS);
    sink += warm_path(WARM_ROUNDS);
    sink += (double)fibonacci(FIB_N);
}

int main(int argc, char *argv[]) {
    int round;

    (void)argc;
    (void)argv;

    printf("libgprof example: running workload (this takes a few seconds)...\n");

    for(round = 0; round < ROUNDS; round++) {
        workload();
        printf("  round %d/%d done\n", round + 1, ROUNDS);
    }

    printf("done (sink = %f)\n", sink);
    printf("exiting -- gmon.out is flushed to /pc on exit\n");

    return 0;
}
