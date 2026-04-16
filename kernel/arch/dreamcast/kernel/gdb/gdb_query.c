/* KallistiOS ##version##

   arch/dreamcast/kernel/gdb/gdb_query.c

   Copyright (C) 2026 Andy Barajas

*/

/*
   Implements GDB query and set-query packet handling.

   Supported query/set-query packets include:
     - qSupported, qTStatus, qOffsets, qAttached, qSymbol
     - qC, qfThreadInfo, qsThreadInfo, qThreadExtraInfo
     - qGetTLSAddr
     - QStartNoAckMode

   Thread queries use live KOS thread metadata, and TLS lookups compute the
   address of the static TLS block for the requested thread.
*/

#include <stdio.h>
#include <inttypes.h>

#include <kos/thread.h>

#include "gdb_internal.h"

extern int _tdata_size, _tbss_size;
extern long _tdata_align, _tbss_align;

typedef struct {
    void *dtv;
    uintptr_t pointer_guard;
} gdb_tcbhead_t;

typedef struct {
    char *out;
    size_t remaining;
    bool first;
} thread_list_state_t;

/* Rounds a size up to the next multiple of the requested alignment. */
static size_t align_to(size_t value, size_t alignment) {
    return (value + (alignment - 1)) & ~(alignment - 1);
}

/* Returns the byte offset from the TLS handle to the thread's static TLS data. */
static size_t tls_static_data_offset(void) {
    const size_t tdata_size = (size_t)(&_tdata_size);
    const size_t tbss_size = (size_t)(&_tbss_size);
    size_t align = 8;

    if(tdata_size && (size_t)_tdata_align > align)
        align = (size_t)_tdata_align;
    if(tbss_size && (size_t)_tbss_align > align)
        align = (size_t)_tbss_align;

    return align_to(sizeof(gdb_tcbhead_t), align);
}

/* Returns whether a query packet must match the given name exactly. */
static bool match_exact_query(const char *ptr, const char *query) {
    return strcmp(ptr, query) == 0;
}

/* Returns whether a query matches a name and an optional separator suffix. */
static bool match_query_with_optional_suffix(const char *ptr,
                                             const char *query,
                                             char suffix_sep) {
    size_t len = strlen(query);

    return strncmp(ptr, query, len) == 0 &&
           (ptr[len] == '\0' || ptr[len] == suffix_sep);
}

/* Parses qSupported features and enables any stub options negotiated by GDB. */
static void parse_qsupported_features(const char *features) {
    const char *ptr = features;

    set_error_messages_enabled(false);

    while(*ptr) {
        if(strncmp(ptr, "error-message+", 14) == 0) {
            set_error_messages_enabled(true);
            break;
        }

        ptr = strchr(ptr, ';');
        if(!ptr)
            break;
        ++ptr;
    }
}

/* Callback for thd_each() for qfThreadInfo packet. */
static int append_thread_id(kthread_t *thd, void *user_data) {
    thread_list_state_t *state = (thread_list_state_t *)user_data;
    char tid_hex[9];
    int len;

    len = format_thread_id_hex(tid_hex, (uint32_t)thd->tid);
    if(len < 0)
        return -1;

    if(state->remaining < (size_t)len + (state->first ? 1u : 2u))
        return -1;

    if(!state->first) {
        *state->out++ = ',';
        --state->remaining;
    }

    memcpy(state->out, tid_hex, (size_t)len + 1u);
    state->out += len;
    state->remaining -= (size_t)len;
    state->first = false;
    return 0;
}

/*
   Handle the 'T' command.

   Checks whether the supplied thread ID names a live KOS thread.
   Format: T<thread-id> where the thread ID is an unpadded hex value.

   The stub replies "OK" only when the parsed thread exists at the time of
   the query. Malformed thread IDs and dead threads both return EINVAL.
*/
void handle_thread_alive(char *ptr) {
    uint32_t tid = 0;

    if(hex_to_int(&ptr, &tid) && *ptr == '\0' && thd_by_tid((tid_t)tid))
        gdb_put_ok();
    else
        gdb_error_with_code_str(GDB_EINVAL, "T: invalid or dead thread");
}

/*
   Handle supported 'q' query packets.

   This dispatcher owns the query subset advertised by the stub, including
   feature negotiation, thread enumeration, thread metadata, and TLS lookups.
   Recognized malformed queries return an explicit error; unrecognized optional
   queries fall back to the normal empty RSP reply.
*/
void handle_query(char *ptr) {
    /*
       Handle the 'qSupported' command.

       Negotiates optional protocol features and reports the capabilities
       supported by this stub.

       PacketSize reports the maximum payload bytes this stub accepts in a
       framed packet, not the total on-wire size including '$', '#', and the
       checksum bytes.
    */
    if(strncmp(ptr, "Supported:", 10) == 0) {
        parse_qsupported_features(ptr + 10);
        snprintf(remcom_out_buffer, BUFMAX,
                 "PacketSize=%x;"
                 "binary-upload+;"
                 "QStartNoAckMode+;"
                 "error-message+;"
                 "vContSupported+;"
                 "swbreak+;"
                 "hwbreak+;",
                 BUFMAX - 4);
        return;
    }

    /*
       Handle the 'qTStatus' command.

       Reports if there is pending asynchronous stop information.
       Format: qTStatus
       Response: Empty means no pending stop.
    */
    if(match_exact_query(ptr, "TStatus")) {
        gdb_clear_out_buffer();
        return;
    }
    /*
       Handle the 'qOffsets' command.

       Requests the memory offsets for text, data, and bss.
       Format: qOffsets
       Response: Text=ADDR;Data=ADDR;Bss=ADDR
    */
    if(match_exact_query(ptr, "Offsets")) {
        gdb_put_str("Text=0;Data=0;Bss=0");
        return;
    }
    /*
       Handle the 'qAttached' command.

       Reports if the debugger was already attached (1) or newly attached (0).
       Format: qAttached
       This stub always reports "1".
    */
    if(match_exact_query(ptr, "Attached")) {
        gdb_put_str("1");
        return;
    }
    /*
       Handle the 'qSymbol' command.

       GDB sends this to initiate or continue symbol lookup negotiation.
       This stub does not request any symbols and simply replies "OK" for the
       exact qSymbol packet and for qSymbol:<payload> continuation packets.
    */
    if(match_query_with_optional_suffix(ptr, "Symbol", ':')) {
        gdb_put_ok();
        return;
    }
    /*
       Handle the 'qC' command.

       Reports the current active thread ID.
       Format: qC
       Response: QC<thread-id> where the thread ID is an unpadded hex value.
    */
    if(match_exact_query(ptr, "C")) {
        kthread_t *thd = thd_get_current();

        remcom_out_buffer[0] = 'Q';
        remcom_out_buffer[1] = 'C';
        format_thread_id_hex(remcom_out_buffer + 2, (uint32_t)thd->tid);
        return;
    }
    /*
       Handle the 'qfThreadInfo' command.

       Returns the initial chunk of the live thread list.
       Format: qfThreadInfo
       Response: m<thread-id>[,<thread-id>...]
    */
    if(match_exact_query(ptr, "fThreadInfo")) {
        thread_list_state_t state;

        remcom_out_buffer[0] = 'm';
        remcom_out_buffer[1] = '\0';

        state.out = remcom_out_buffer + 1;
        state.remaining = BUFMAX - 1;
        state.first = true;

        if(thd_each(append_thread_id, &state) < 0)
            gdb_error_with_code_str(GDB_EGENERIC,
                                    "qfThreadInfo: response too large");

        return;
    }
    /*
       Handle the 'qsThreadInfo' command.

       Returns continuation thread list data after a 'qfThreadInfo' packet.
       Format: qsThreadInfo

       This implementation does not paginate thread IDs across multiple
       responses. If the initial qfThreadInfo reply succeeds, qsThreadInfo
       always returns 'l' to indicate the end of the list.
    */
    if(match_exact_query(ptr, "sThreadInfo")) {
        strcpy(remcom_out_buffer, "l");
        return;
    }
    /*
       Handle the 'qThreadExtraInfo' command.

       Provides hex-encoded human-readable information about a specific thread.
       Format: qThreadExtraInfo,<thread-id>

       This implementation returns the thread label when one is available and
       an empty reply for unlabeled threads. GDB may display the text in thread
       listings.

       Example:
         Request:  qThreadExtraInfo,04
         Response: 6D61696E20746872656164   ("main thread")
    */
    if(strncmp(ptr, "ThreadExtraInfo,", 16) == 0) {
        uint32_t tid = 0;

        ptr += 16;
        if(hex_to_int(&ptr, &tid) && *ptr == '\0') {
            kthread_t *thd = thd_by_tid((tid_t)tid);

            if(thd) {
                const char *label = thd_get_label(thd);

                if(label)
                    mem_to_hex(label, remcom_out_buffer, strlen(label));
                else
                    gdb_clear_out_buffer();
            }
            else {
                gdb_error_with_code_str(GDB_EINVAL,
                                        "qThreadExtraInfo: unknown thread");
            }
        }
        else {
            gdb_error_with_code_str(GDB_EINVAL,
                                    "qThreadExtraInfo: invalid packet");
        }

        return;
    }
    /*
       Handle the 'qGetTLSAddr' command.

       Returns the address of a TLS variable for a specific thread.
       Format: qGetTLSAddr:TID,OFFSET,LMID
        - TID: Thread ID
        - OFFSET: Offset within the thread's static TLS data block
        - LMID: Link map ID (ignored in KOS)
       Response: hex-encoded target address of the requested TLS location

       KOS computes this as the thread's TLS handle plus the static TLS data
       offset that follows the local TCB header.
    */
    if(strncmp(ptr, "GetTLSAddr:", 11) == 0) {
        uint32_t tid = 0;
        uint32_t offset = 0;
        uint32_t lmid = 0;

        ptr += 11;
        if(hex_to_int(&ptr, &tid) && *ptr++ == ',' &&
           hex_to_int(&ptr, &offset) && *ptr++ == ',' &&
           hex_to_int(&ptr, &lmid) && *ptr == '\0') {
            kthread_t *thd = thd_by_tid((tid_t)tid);

            (void)lmid;

            if(thd && thd->tls_hnd) {
                uintptr_t tls_addr =
                    (uintptr_t)thd->tls_hnd + tls_static_data_offset() + offset;
                snprintf(remcom_out_buffer, BUFMAX, "%0*" PRIxPTR,
                         (int)(sizeof(tls_addr) * 2u), tls_addr);
            }
            else {
                gdb_error_with_code_str(GDB_EINVAL,
                                        "qGetTLSAddr: unavailable TLS for thread");
            }
        }
        else {
            gdb_error_with_code_str(GDB_EINVAL,
                                    "qGetTLSAddr: invalid packet");
        }

        return;
    }

    remcom_out_buffer[0] = '\0';
}

/*
   Handle the 'Q' command.

   Handles supported set-query packets that change stub behavior.

   Currently supported:
     - QStartNoAckMode

   QStartNoAckMode switches the transport into no-ack mode after replying
   "OK". Unsupported Q packets return an empty reply rather than an error.
*/
void handle_set_query(char *ptr) {
    if(match_exact_query(ptr, "StartNoAckMode")) {
        set_no_ack_mode_enabled(true);
        gdb_put_ok();
    }
    else {
        gdb_clear_out_buffer();
    }
}
