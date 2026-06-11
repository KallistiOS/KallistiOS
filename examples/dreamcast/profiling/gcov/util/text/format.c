/* KallistiOS ##version##

   util/text/format.c
   Copyright (C) 2026 Andress Barajas

*/

#include "format.h"

const char *fmt_parity(int n) {
    if(n == 0)
        return "zero";
    if(n & 1)
        return "odd";
    return "even";
}
