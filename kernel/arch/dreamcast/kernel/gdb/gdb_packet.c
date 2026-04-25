/* KallistiOS ##version##

   arch/dreamcast/kernel/gdb/gdb_packet.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2026 Andy Barajas

*/

/*
   Implements the transport layer for the GDB remote serial protocol.

   This module handles:
     - packet framing and checksum validation
     - optional no-ack mode (QStartNoAckMode)
     - run-length encoding for outbound replies
     - runtime use of either dc-load or SCIF transports

   The current encoder avoids count bytes that would collide with packet
   framing characters(#, $) on the wire.
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

/* Returns a pointer to the GDB output buffer. */
char *gdb_get_out_buffer(void) {
    return remcom_out_buffer;
}

/* Returns the exact byte length of the most recently received packet payload. */
size_t gdb_get_in_packet_length(void) {
    return remcom_in_length;
}

/* Clears the GDB output buffer by setting the first byte to null. */
void gdb_clear_out_buffer(void) {
    remcom_out_buffer[0] = '\0';
}

/* Writes an "OK" response to the GDB output buffer. */
void gdb_put_ok(void) {
    strcpy(remcom_out_buffer, GDB_OK);
}

/* Writes a custom string response to the GDB output buffer. */
void gdb_put_str(const char *msg) {
    strcpy(remcom_out_buffer, msg);
}

/* Enable or disable textual E.<message> replies after qSupported negotiation. */
void set_error_messages_enabled(bool enabled) {
    error_messages_enabled = enabled;
}

/* Enable or disable RSP no-ack mode after QStartNoAckMode negotiation. */
void set_no_ack_mode_enabled(bool enabled) {
    no_ack_mode = enabled;
}

/*
   Formats an error reply for the current packet.

   When textual error replies are enabled, this writes an E.<message> response.
   Otherwise it falls back to the supplied machine-readable Exx code.
*/
void gdb_error_with_code_str(const char *errcode, const char *msg_fmt, ...) {
    va_list args;

    if(error_messages_enabled) {
        int written;

        strcpy(remcom_out_buffer, "E.");

        /* Format the textual error payload after the E. prefix. */
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

/*
   Reads one byte from the debug channel.
   Uses DCLoad or SCIF depending on context.
   Blocks until data is available.
*/
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

/*
   Sends a single character over the debug channel.
   Buffered when using DCLoad; flushed immediately on SCIF.
*/
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

/*
   Flushes the output buffer to the host.

   For dc-load, this sends any buffered output and may also refill the inbound
   packet buffer. For SCIF, per-character writes are already flushed, so this
   helper is effectively a no-op.
*/
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
   Returns the longest run that should be emitted using RSP run-length encoding.

   Runs shorter than four bytes are left uncompressed. The returned run length
   is also constrained so that the encoded count byte does not collide with
   packet framing characters on the wire.
*/
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

/*
   Discards the remainder of the current packet after the local input buffer
   has filled, including the trailing checksum bytes.
*/
static void discard_packet_tail(void) {
    char ch;

    do {
        ch = get_debug_char();
    } while(ch != '#');

    /*
       These two calls consume the 2 checksum bytes that come after # in a GDB
       RSP packet
    */
    (void)get_debug_char();
    (void)get_debug_char();
}

/*
   Reads a full GDB packet from the debug channel.

   Format: $<data>#<checksum>
   Verifies the checksum and emits +/- acknowledgements when ack mode is active.
   If a sequence prefix is present, the returned pointer skips past it.
*/
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
                if(!no_ack_mode) {
                    put_debug_char('-');    /* failed checksum */
                    flush_debug_channel();
                }
            }
            else {
                if(!no_ack_mode)
                    put_debug_char('+');    /* successful transfer */

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

/*
   Sends a GDB response packet using optional run-length encoding.

   Format: $<data>#<checksum>
   Retransmits until the host sends an ACK, unless no-ack mode is active.
*/
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
