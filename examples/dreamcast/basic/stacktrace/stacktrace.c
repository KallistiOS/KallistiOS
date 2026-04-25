/* KallistiOS ##version##

   stacktrace.c
   Copyright (c) 2002 Megan Potter
   Copyright (c) 2026 Andy Barajas

   -------------------
   Demonstrates arch_stk_trace() by printing stack traces from three
   different call scenarios:

   1. Directly from main() -- a shallow trace.
   2. From inside a recursive function (depth_recurse, 6 levels deep).
   3. From inside a function called through a function pointer.

   The volatile arrays and fill loops in these functions are intentional
   -- they force the compiler to create real stack frames that save PR
   (the return address register) to the stack. Without them, GCC for
   SH4 keeps everything in registers and the stack scanner has nothing
   to find.

   Expected output (addresses will vary):

   -------- Stack Trace (innermost first) ---------
      8c010088
   -------------- End Stack Trace -----------------
   -------- Stack Trace (innermost first) ---------
      8c010152
      8c010152
      8c010152
      8c010152
      8c010196
      8c01024a
      8c010684
      8c010088
   -------------- End Stack Trace -----------------
   -------- Stack Trace (innermost first) ---------
      8c01022c
      8c010250
      8c010684
      8c010088
   -------------- End Stack Trace -----------------
*/

#include <kos.h>

/* A simple function that sums its input with an array.
   Called through a function pointer to test indirect calls (JSR).
   The volatile array and loop force a real stack frame.
   The stack trace is taken here so the full indirect call chain
   (do_work -> call_indirect -> test_indirect -> main) is visible. */
__noinline static int do_work(int x) {
    volatile int buf[12];
    int i;

    for(i = 0; i < 12; ++i)
        buf[i] = x + i;

    arch_stk_trace(0);
    return buf[0] + buf[11];
}

/* Function pointer type for do_work. */
typedef int (*work_fn)(int);

/* Calls a function through a function pointer and returns the result.
   Tests that indirect calls show up in traces. */
__noinline static int call_indirect(work_fn fn, int v) {
    volatile int buf[12];
    int i;

    for(i = 0; i < 12; ++i)
        buf[i] = v + i;

    buf[0] = fn(buf[0]);
    return buf[0] + buf[11];
}

/* A recursive function that calls itself `depth` times, then prints
   a stack trace at the deepest level. The volatile array ensures each
   recursion level gets a real stack frame with a saved PR value. The
   addition after the recursive call prevents tail-call optimization. */
__noinline static int depth_recurse(int depth, int value) {
    volatile int buf[12];
    int i;

    for(i = 0; i < 12; ++i)
        buf[i] = value + depth + i;

    if(depth == 0) {
        arch_stk_trace(0);
        return buf[0];
    }

    return depth_recurse(depth - 1, buf[0]) + buf[1];
}

/* Trigger a stack trace from 6 levels of recursion. */
__noinline static void test_recursive(void) {
    volatile int buf[12];
    int i;

    for(i = 0; i < 12; ++i)
        buf[i] = i;

    buf[0] = depth_recurse(5, buf[0]);
    (void)buf[0];
}

/* Call a function through a function pointer. The stack trace is
   taken inside do_work so the full call chain is on the stack. */
__noinline static void test_indirect(void) {
    volatile int buf[12];
    int i;

    for(i = 0; i < 12; ++i)
        buf[i] = i * 2;

    buf[0] = call_indirect(do_work, buf[0]);
    (void)buf[0];
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    /* Stack trace directly from main. */
    arch_stk_trace(0);

    /* Stack trace from deep recursion. */
    test_recursive();

    /* Stack trace from inside a function called through a pointer. */
    test_indirect();

    printf("stacktrace example done.\n");

    return 0;
}
