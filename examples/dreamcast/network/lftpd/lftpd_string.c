/* KallistiOS ##version##

   lftpd_string.c
   Copyright (C) 2018 Jason von Nieda
   Copyright (C) 2026 Andy Barajas
*/

/* String helpers for lftpd. */

#include <string.h>
#include <ctype.h>

#include "private/lftpd_string.h"

char *lftpd_string_trim(char *s) {
    /* Trim trailing whitespace in place. */
    size_t len = strlen(s);
    while(len > 0 && isspace((unsigned char) s[len - 1])) {
        s[--len] = '\0';
    }

    /* Trim leading whitespace by shifting the remainder down, so the returned
       pointer is still the start of the caller's allocation (it gets free()d). */
    size_t start = 0;
    while(s[start] && isspace((unsigned char) s[start])) {
        start++;
    }
    if(start > 0) {
        memmove(s, s + start, len - start + 1);
    }

    return s;
}
