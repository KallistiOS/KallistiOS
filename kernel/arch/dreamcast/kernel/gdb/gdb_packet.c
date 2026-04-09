/* KallistiOS ##version##

   kernel/gdb/gdb_packet.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2026 Andy Barajas

*/

#include <dc/dcload.h>
#include <dc/scif.h>

#include "gdb_internal.h"

int using_dcl = 0;
char remcom_out_buffer[BUFMAX];

static char in_dcl_buf[BUFMAX];
static char out_dcl_buf[BUFMAX];
static uint32_t in_dcl_pos = 0;
static uint32_t out_dcl_pos = 0;
static uint32_t in_dcl_size = 0;
static char remcom_in_buffer[BUFMAX];

static char get_debug_char(void) {
    int ch;

    if(using_dcl) {
        if(in_dcl_pos >= in_dcl_size) {
            in_dcl_size = dcload_gdbpacket(NULL, 0, in_dcl_buf, BUFMAX);
            in_dcl_pos = 0;
        }

        ch = in_dcl_buf[in_dcl_pos++];
    }
    else {
        /* Spin while nothing is available. */
        while((ch = scif_read()) < 0);

        ch &= 0xff;
    }

    return ch;
}

static void put_debug_char(char ch) {
    if(using_dcl) {
        out_dcl_buf[out_dcl_pos++] = ch;

        if(out_dcl_pos >= BUFMAX) {
            dcload_gdbpacket(out_dcl_buf, out_dcl_pos, NULL, 0);
            out_dcl_pos = 0;
        }
    }
    else {
        /* write the char and flush it. */
        scif_write(ch);
        scif_flush();
    }
}

static void flush_debug_channel(void) {
    /* send the current complete packet and wait for a response */
    if(using_dcl) {
        if(in_dcl_pos >= in_dcl_size) {
            in_dcl_size = dcload_gdbpacket(out_dcl_buf, out_dcl_pos, in_dcl_buf, BUFMAX);
            in_dcl_pos = 0;
        }
        else
            dcload_gdbpacket(out_dcl_buf, out_dcl_pos, NULL, 0);

        out_dcl_pos = 0;
    }
}

/*
 * Routines to get and put packets
 */

/* scan for the sequence $<data>#<checksum>     */

unsigned char *get_packet(void) {
    unsigned char *buffer = (unsigned char *)(&remcom_in_buffer[0]);
    unsigned char checksum;
    unsigned char xmitcsum;
    int count;
    char ch;

    while(true) {
        /* wait around for the start character, ignore all other characters */
        while((ch = get_debug_char()) != '$')
            ;

    retry:
        checksum = 0;
        xmitcsum = -1;
        count = 0;

        /* now, read until a # or end of buffer is found */
        while(count < (BUFMAX-1)) {
            ch = get_debug_char();

            if(ch == '$')
                goto retry;

            if(ch == '#')
                break;

            checksum = checksum + ch;
            buffer[count] = ch;
            count = count + 1;
        }

        buffer[count] = 0;

        if(ch == '#') {
            ch = get_debug_char();
            xmitcsum = hex(ch) << 4;
            ch = get_debug_char();
            xmitcsum += hex(ch);

            if(checksum != xmitcsum) {
                put_debug_char('-');    /* failed checksum */
            }
            else {
                put_debug_char('+');    /* successful transfer */

//        printf("get_packet() -> %s\n", buffer);

                /* if a sequence char is present, reply the sequence ID */
                if(buffer[2] == ':') {
                    put_debug_char(buffer[0]);
                    put_debug_char(buffer[1]);

                    return &buffer[3];
                }

                return &buffer[0];
            }
        }
    }
}


/* send the packet in buffer. */
void put_packet(const char *buffer) {
    int check_sum;

    /*  $<packet info>#<checksum>. */
    do {
        const char *src = buffer;
        put_debug_char('$');
        check_sum = 0;

        while(*src) {
            int runlen;

            /* Do run length encoding */
            for(runlen = 0; runlen < 100; runlen ++) {
                if(src[0] != src[runlen] || runlen == 99) {
                    if(runlen > 3) {
                        int encode;
                        /* Got a useful amount */
                        put_debug_char(*src);
                        check_sum += *src;
                        put_debug_char('*');
                        check_sum += '*';
                        check_sum += (encode = runlen + ' ' - 4);
                        put_debug_char(encode);
                        src += runlen;
                    }
                    else {
                        put_debug_char(*src);
                        check_sum += *src;
                        src++;
                    }

                    break;
                }
            }
        }

        put_debug_char('#');
        put_debug_char(highhex(check_sum));
        put_debug_char(lowhex(check_sum));
        flush_debug_channel();
    } while(get_debug_char() != '+');
}
