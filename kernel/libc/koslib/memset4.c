/* KallistiOS ##version##

   memset4.c
   (c)2000 Megan Potter

*/

#include <string.h>

/* This variant was added by Megan Potter for its usefulness in
   working with Dreamcast external hardware. */
void * memset4(void *s, unsigned long c, size_t count) {
    unsigned long *xs = (unsigned long *) s;
    count = count / 4;

    while(count--)
        *xs++ = c;

    return s;
}

