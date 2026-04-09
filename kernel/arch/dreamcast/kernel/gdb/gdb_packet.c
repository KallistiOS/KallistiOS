/* KallistiOS ##version##

   kernel/gdb/gdb_packet.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2026 Andy Barajas

*/

#include <dc/dcload.h>
#include <dc/scif.h>

#include <stdarg.h>
#include <stdio.h>

#include "gdb_internal.h"

int using_dcl = 0;
char remcom_out_buffer[BUFMAX];
static bool no_ack_mode;
static bool error_messages_enabled;

static char in_dcl_buf[BUFMAX];
static char out_dcl_buf[BUFMAX];
static uint32_t in_dcl_pos = 0;
static uint32_t out_dcl_pos = 0;
static uint32_t in_dcl_size = 0;
static char remcom_in_buffer[BUFMAX];
static size_t remcom_in_length;

char *gdb_get_out_buffer(void) {
    return remcom_out_buffer;
}

size_t gdb_get_in_packet_length(void) {
    return remcom_in_length;
}

void gdb_clear_out_buffer(void) {
    remcom_out_buffer[0] = '\0';
}

void gdb_put_ok(void) {
    strcpy(remcom_out_buffer, GDB_OK);
}

void gdb_put_str(const char *msg) {
    strcpy(remcom_out_buffer, msg);
}

void set_error_messages_enabled(bool enabled) {
    error_messages_enabled = enabled;
}

void set_no_ack_mode_enabled(bool enabled) {
    no_ack_mode = enabled;
}

void gdb_error_with_code_str(const char *errcode, const char *msg_fmt, ...) {
    va_list args;

    if(error_messages_enabled) {
        int written;

        strcpy(remcom_out_buffer, "E.");
        va_start(args, msg_fmt);
        written = vsnprintf(remcom_out_buffer + 2, BUFMAX - 2, msg_fmt, args);
        va_end(args);

        if(written < 0)
            strncpy(remcom_out_buffer, errcode, BUFMAX - 1);
    }
    else {
        strncpy(remcom_out_buffer, errcode, BUFMAX - 1);
    }

    remcom_out_buffer[BUFMAX - 1] = '\0';
}

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

static int get_rle_runlen(const char *src) {
    int runlen = 1;

    while(runlen < 99 && src[runlen] == src[0])
        ++runlen;

    if(runlen > 98)
        runlen = 98;

    /* The RLE count byte is runlen + (' ' - 4), so runs of 7 and 8 would
       encode to '#' and '$'. Break those into smaller chunks instead. */
    if(runlen == 7 || runlen == 8)
        runlen = 6;

    return runlen > 3 ? runlen : 0;
}

static void discard_packet_tail(void) {
    char ch;

    do {
        ch = get_debug_char();
    } while(ch != '#');

    (void)get_debug_char();
    (void)get_debug_char();
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
        remcom_in_length = 0;

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

        if(count >= (BUFMAX - 1) && ch != '#') {
            discard_packet_tail();

            if(!no_ack_mode) {
                put_debug_char('-');
                flush_debug_channel();
            }

            continue;
        }

        if(ch == '#') {
            ch = get_debug_char();
            xmitcsum = hex(ch) << 4;
            ch = get_debug_char();
            xmitcsum += hex(ch);

            if(checksum != xmitcsum) {
                if(!no_ack_mode)
                    put_debug_char('-');    /* failed checksum */
            }
            else {
                if(!no_ack_mode)
                    put_debug_char('+');    /* successful transfer */

//        printf("get_packet() -> %s\n", buffer);

                /* if a sequence char is present, reply the sequence ID */
                if(count > 2 && buffer[2] == ':') {
                    remcom_in_length = (size_t)count - 3u;
                    put_debug_char(buffer[0]);
                    put_debug_char(buffer[1]);

                    return &buffer[3];
                }

                remcom_in_length = (size_t)count;
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
            int runlen = get_rle_runlen(src);

            if(runlen > 0) {
                int encode = runlen + ' ' - 4;

                put_debug_char(*src);
                check_sum += *src;
                put_debug_char('*');
                check_sum += '*';
                put_debug_char(encode);
                check_sum += encode;
                src += runlen;
            }
            else {
                put_debug_char(*src);
                check_sum += *src;
                ++src;
            }
        }

        put_debug_char('#');
        put_debug_char(highhex(check_sum));
        put_debug_char(lowhex(check_sum));
        flush_debug_channel();

        if(no_ack_mode)
            break;
    } while(get_debug_char() != '+');
}
