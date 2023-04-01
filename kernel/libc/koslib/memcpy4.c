/* KallistiOS ##version##

   memcpy4.c
   (c)2000 Megan Potter

*/

#include <string.h>

/* This variant was added by Megan Potter for its usefulness in
   working with Dreamcast external hardware. */
void * memcpy4(void *dest, const void *src, size_t count) {
    unsigned long *tmp = (unsigned long *) dest;
    unsigned long *s = (unsigned long *) src;
    count = count / 4;

    while(count--)
        *tmp++ = *s++;

    return dest;
}
