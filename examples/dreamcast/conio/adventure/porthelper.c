/* KallistiOS ##version##

   porthelper.c

   (c)2002 Megan Potter

 */

#include "porthelper.h"

static char buffer[256];
static int have_buffer = 0, bufptr = -1, buflen = -1;

int getchar() {
    int rv;

    if(!have_buffer) {
        conio_input_getline(-1, buffer, 256);
        bufptr = 0;
        strcat(buffer, "\n");
        buflen = strlen(buffer);
        have_buffer = 1;
    }

    rv = buffer[bufptr++];

    if(bufptr >= buflen) {
        have_buffer = 0;
    }

    return rv;
}

int putchar(int c) {
    if(c == '\t') {
        conio_printf("        ");
        return;
    }

    if(c != '\r' && c != '\n' && (c < 32 || c > 127))
        c = '.';

    conio_printf("%c", c);
    return 0;
}

long random() {
    return rand();
}
