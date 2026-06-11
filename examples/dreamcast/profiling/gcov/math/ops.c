/* KallistiOS ##version##

   math/ops.c
   Copyright (C) 2026 Andress Barajas

*/

#include "ops.h"

int op_clamp(int v, int lo, int hi) {
    if(v < lo)
        return lo;
    if(v > hi)
        return hi;
    return v;
}

int op_sign(int v) {
    if(v > 0)
        return 1;
    else if(v < 0)
        return -1;
    return 0;
}
