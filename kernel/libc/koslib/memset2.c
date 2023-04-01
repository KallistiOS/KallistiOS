/* KallistiOS ##version##

   memset2.c
   (c)2001 Megan Potter

*/

#include <string.h>

/* This variant was added by Megan Potter for its usefulness in
   working with GBA external hardware. */
void * memset2(void *s, unsigned short c, size_t count) {
    unsigned short *xs = (unsigned short *) s;
    count = count / 2;

    while(count--)
        *xs++ = c;

    return s;
}

