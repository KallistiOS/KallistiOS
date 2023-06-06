/* KallistiOS ##version##

   inet_ntoa.c
   Copyright (C) 2007 Lawrence Sebald

*/

#include <arpa/inet.h>

char *inet_ntoa(struct in_addr addr) {
    static char str[16]; /* XXX.XXX.XXX.XXX = 15 chars + 1 for NUL */
    char tmp[3];
    int i, part;
    char *ch = tmp;
    char *ch2 = str;

    /* Parse each 8 bits individually. */
    for(i = 0; i < 4; ++i) {
        /* Treat the 32-bit address value as if it were an array of 8-bit
           values. This works, regardless of the endianness of the host system
           because the specs require the address passed in here to be in
           network byte order (big endian). */
        part = ((uint8_t *) &addr.s_addr)[i];

        do {
            *ch++ = '0' + (char)(part % 10);
            part /= 10;
        }
        while(part);

        /* tmp now contains the inverse of the number that is in the given
           8 bits. Reverse it for the final result, rewinding ch to the
           beginning of tmp in the process. */
        while(ch != tmp) {
            *ch2++ = *--ch;
        }

        *ch2++ = '.';
    }

    /* There's a trailing '.' at the end of the address, change it to the
       required NUL character */
    *--ch2 = 0;

    return str;
}
