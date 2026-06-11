/* KallistiOS ##version##

   util/str.c
   Copyright (C) 2026 Andress Barajas

*/

#include "str.h"

int str_len(const char *s) {
    int n = 0;
    while(s && *s) {
        n++;
        s++;
    }
    return n;
}

int str_count(const char *s, char c) {
    int n = 0;
    if(!s)
        return 0;
    for(; *s; s++) {
        if(*s == c)
            n++;
    }
    return n;
}
