/* KallistiOS ##version##

   kernel/gdb/gdb_utils.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2026 Andy Barajas

*/

#include <stdio.h>

#include "gdb_internal.h"

static const char hexchars[] = "0123456789abcdef";

int format_thread_id_hex(char out[9], uint32_t tid) {
    return snprintf(out, 9, "%x", (unsigned int)tid);
}

char highhex(int x) {
    return hexchars[(x >> 4) & 0xf];
}

char lowhex(int x) {
    return hexchars[x & 0xf];
}

int hex(char ch) {
    if((ch >= 'a') && (ch <= 'f'))
        return (ch - 'a' + 10);

    if((ch >= '0') && (ch <= '9'))
        return (ch - '0');

    if((ch >= 'A') && (ch <= 'F'))
        return (ch - 'A' + 10);

    return -1;
}

/*
   Convert binary data to a hex string.

   This function converts 'count' bytes from the binary data pointed to by 'mem'
   into a hex string and stores it in 'buf'. It returns a pointer to the character
   in 'buf' immediately after the last written character (null-terminator).

   mem     Pointer to the binary data.
   buf     Pointer to the output buffer for the hex string.
   count   Number of bytes to convert.
 */
char *mem_to_hex(const char *src, char *dest, size_t count) {
    size_t i;
    int ch;

    for(i = 0; i < count; i++) {
        ch = *src++;
        *dest++ = highhex(ch);
        *dest++ = lowhex(ch);
    }
    *dest = 0;

    return dest;
}

/*
   Convert a hex string to binary data.

   This function converts 'count' bytes from the hex string 'buf' into binary
   data and stores it in 'mem'. It returns a pointer to the character in 'mem'
   immediately after the last byte written.

    buf     Pointer to the hex string.
    mem     Pointer to the output buffer for binary data.
    count   Number of bytes to convert (half the length of 'buf').
 */
char *hex_to_mem(const char *src, char *dest, size_t count) {
    uint32_t i;
    unsigned char high;
    unsigned char low;

    for(i = 0; i < count; i++) {
        high = hex(*src++);
        low  = hex(*src++);
        *dest++ = (high << 4) | low;
    }

    return dest;
}

/*
   Convert a hex string to an integer value.

   This function reads hexadecimal digits from the string pointed to by `*ptr`
   and accumulates them into an integer. It updates `*int_value` with the
   result and advances `*ptr` to the first non-hex character.

   ptr        Pointer to a char pointer that will be advanced past the parsed digits.
   int_value  Output parameter to store the resulting integer value.

   Returns the number of hex digits processed.
 */
size_t hex_to_int(char **ptr, uint32_t *int_value) {
    size_t num_chars = 0;
    int hex_value;

    if(!ptr || !*ptr || !int_value)
        return 0;

    *int_value = 0;

    while(**ptr) {
        hex_value = hex(**ptr);

        if(hex_value >= 0) {
            *int_value = (*int_value << 4) | hex_value;
            num_chars++;
        }
        else
            break;

        (*ptr)++;
    }

    return num_chars;
}
