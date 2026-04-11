/* KallistiOS ##version##

   kernel/gdb/gdb_query.c

   Copyright (C) 2026 Andy Barajas

*/

#include <stdio.h>

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

static size_t align_to(size_t value, size_t alignment) {
    return (value + (alignment - 1)) & ~(alignment - 1);
}

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

static int append_thread_id(kthread_t *thd, void *user_data) {
    thread_list_state_t *state = (thread_list_state_t *)user_data;
    char tid_hex[9];
    int len;

    len = format_thread_id_hex(tid_hex, (uint32_t)thd->tid);
    if(len < 0)
        return -1;

    if(state->remaining <= (size_t)len + (state->first ? 1u : 2u))
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

void handle_thread_alive(char *ptr) {
    uint32_t tid = 0;

    if(hex_to_int(&ptr, &tid) && *ptr == '\0' && thd_by_tid((tid_t)tid))
        strcpy(remcom_out_buffer, GDB_OK);
    else
        strcpy(remcom_out_buffer, "E01");
}

void handle_query(char *ptr) {
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

    if(strncmp(ptr, "TStatus", 7) == 0) {
        gdb_clear_out_buffer();
        return;
    }

    if(strncmp(ptr, "Offsets", 7) == 0) {
        gdb_put_str("Text=0;Data=0;Bss=0");
        return;
    }

    if(strncmp(ptr, "Attached", 8) == 0) {
        gdb_put_str("1");
        return;
    }

    if(strncmp(ptr, "Symbol", 6) == 0) {
        gdb_put_ok();
        return;
    }

    if(*ptr == 'C') {
        kthread_t *thd = thd_get_current();

        remcom_out_buffer[0] = 'Q';
        remcom_out_buffer[1] = 'C';
        format_thread_id_hex(remcom_out_buffer + 2, (uint32_t)thd->tid);
        return;
    }

    if(strncmp(ptr, "fThreadInfo", 11) == 0) {
        thread_list_state_t state;

        remcom_out_buffer[0] = 'm';
        remcom_out_buffer[1] = '\0';

        state.out = remcom_out_buffer + 1;
        state.remaining = BUFMAX - 1;
        state.first = true;

        if(thd_each(append_thread_id, &state) < 0)
            strcpy(remcom_out_buffer, "E01");

        return;
    }

    if(strncmp(ptr, "sThreadInfo", 11) == 0) {
        strcpy(remcom_out_buffer, "l");
        return;
    }

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
                strcpy(remcom_out_buffer, "E01");
            }
        }
        else {
            strcpy(remcom_out_buffer, "E01");
        }

        return;
    }

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
                mem_to_hex((const char *)&tls_addr, remcom_out_buffer,
                           sizeof(tls_addr));
            }
            else {
                strcpy(remcom_out_buffer, "E01");
            }
        }
        else {
            strcpy(remcom_out_buffer, "E01");
        }

        return;
    }

    remcom_out_buffer[0] = '\0';
}

void handle_set_query(char *ptr) {
    if(strncmp(ptr, "StartNoAckMode", 14) == 0) {
        set_no_ack_mode_enabled(true);
        gdb_put_ok();
    }
    else {
        gdb_clear_out_buffer();
    }
}
