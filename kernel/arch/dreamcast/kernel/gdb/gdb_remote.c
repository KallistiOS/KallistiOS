/* KallistiOS ##version##

   kernel/gdb/gdb_remote.c

   Copyright (C) 2026 Andy Barajas

*/

#include <arch/arch.h>

#include "gdb_internal.h"

#define SIGILL  4
#define SIGTRAP 5
#define SIGBUS  7
#define SIGSEGV 10

static int compute_signal(int exception_vector) {
    switch(exception_vector) {
        case EXC_ILLEGAL_INSTR:
        case EXC_SLOT_ILLEGAL_INSTR:
            return SIGILL;
        case EXC_USER_BREAK_PRE:
        case EXC_TRAPA:
            return SIGTRAP;
        case EXC_DATA_ADDRESS_READ:
        case EXC_DATA_ADDRESS_WRITE:
            return SIGSEGV;
        default:
            return SIGBUS;
    }
}

static const char *stop_reason_name(int exception_vector) {
    switch(exception_vector) {
        case EXC_TRAPA:
            return "swbreak";
        case EXC_USER_BREAK_PRE:
            return "hwbreak";
        case EXC_DATA_ADDRESS_WRITE:
            return "watch";
        case EXC_DATA_ADDRESS_READ:
            return "rwatch";
        default:
            return "signal";
    }
}

void handle_detach(void) {
    put_packet(GDB_OK);
    gdb_set_connected(false);
}

void handle_kill(void) {
    put_packet(GDB_OK);
    arch_abort();
}

void handle_t_stop_reply(int exception_vector) {
    const irq_context_t *context = gdb_get_irq_context();
    kthread_t *thd = thd_get_current();
    char *out = remcom_out_buffer;
    int sigval = compute_signal(exception_vector);

    *out++ = 'T';
    *out++ = highhex(sigval);
    *out++ = lowhex(sigval);
    out = append_regs(out, context);

    memcpy(out, "thread:", 7);
    out += 7;
    out += format_thread_id_hex(out, (uint32_t)thd->tid);
    *out++ = ';';

    memcpy(out, "reason:", 7);
    out += 7;
    out += strlen(strcpy(out, stop_reason_name(exception_vector)));
    *out++ = ';';
    *out = '\0';
}
