/* KallistiOS ##version##

   gcov.c
   Copyright (C) 2026 Andress Barajas

   Multi-directory code-coverage on the Dreamcast.

   The sources live in several directories (math/, util/, util/text/), so each
   gets its own .gcda whose path reflects where it was built. There is nothing
   coverage-specific in the code itself -- building with --coverage is all it
   takes (kos-cc instruments, links GCC's libgcov + libkosgcov, and the addon
   flushes .gcda on exit). See README.md.
*/

#include <stdio.h>

#include "math/ops.h"
#include "util/str.h"
#include "util/text/format.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    long acc = 0;
    int i;

    printf("gcov multi-directory coverage example\n");

    /* Exercise math/ + util/text/. Some arms stay cold on purpose. */
    for(i = -3; i <= 5; i++) {
        printf("  clamp(%d,0,3)=%d sign=%2d parity=%s\n",
               i, op_clamp(i, 0, 3), op_sign(i), fmt_parity(i));
        acc += op_clamp(i, 0, 3);
    }

    /* Exercise util/. */
    const char *msg = "dreamcast coverage";
    printf("  str_len(\"%s\")=%d, 'a' count=%d\n",
           msg, str_len(msg), str_count(msg, 'a'));
    printf("  acc=%ld\n", acc);

    printf("done -- coverage flushes on exit (see README.md)\n");
    return 0;
}
