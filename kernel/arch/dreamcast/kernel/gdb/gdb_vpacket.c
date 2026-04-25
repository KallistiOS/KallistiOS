/* KallistiOS ##version##

   arch/dreamcast/kernel/gdb/gdb_vpacket.c

   Copyright (C) 2026 Andy Barajas

*/

/*
   Implements extended 'v' packet handling for the GDB remote stub.

   Supported subcommands:
     - vCont?      : advertise supported vCont actions
     - vCont;c     : continue execution
     - vCont;s     : single-step execution
     - vCont;{c,s}[:thread-id] for the currently stopped thread only
     - vMustReplyEmpty
     - vCtrlC
     - vKill[;pid]
*/

#include <arch/arch.h>

#include "gdb_internal.h"

typedef struct {
    char type;
    int tid;
    bool has_thread;
} vcont_action_t;

/* Returns whether a vCont thread selector can target the currently stopped thread. */
static bool is_supported_vcont_thread(int tid) {
    kthread_t *current = thd_get_current();

    if(tid == GDB_THREAD_ANY || tid == GDB_THREAD_ALL)
        return true;

    if(!current || tid <= GDB_THREAD_ANY)
        return false;

    return current->tid == (tid_t)tid;
}

/* Parses a vCont thread-id suffix into one of the stub's thread selectors. */
static bool parse_vcont_thread_id(char *ptr, int *tid) {
    uint32_t parsed_tid = 0;

    if(ptr[0] == '-' && ptr[1] == '1' && ptr[2] == '\0') {
        *tid = GDB_THREAD_ALL;
        return true;
    }

    if(!hex_to_int(&ptr, &parsed_tid) || *ptr != '\0')
        return false;

    *tid = (int)parsed_tid;

    if(*tid > GDB_THREAD_ANY && !thd_by_tid((tid_t)*tid))
        return false;

    return true;
}

/* Parses one vCont action token such as c, s, or s:thread-id. */
static bool parse_vcont_action(char *token, vcont_action_t *action) {
    memset(action, 0, sizeof(vcont_action_t));
    action->tid = GDB_THREAD_ANY;

    if(*token == '\0') {
        gdb_error_with_code_str(GDB_EINVAL, "vCont: empty action");
        return false;
    }

    action->type = *token++;

    if(action->type != 'c' && action->type != 's') {
        gdb_error_with_code_str(GDB_EUNIMPL, "vCont: unsupported action");
        return false;
    }

    if(*token == '\0')
        return true;

    if(*token != ':') {
        gdb_error_with_code_str(GDB_EINVAL, "vCont: invalid action");
        return false;
    }

    if(!parse_vcont_thread_id(token + 1, &action->tid)) {
        gdb_error_with_code_str(GDB_EINVAL, "vCont: invalid thread-id");
        return false;
    }

    if(!is_supported_vcont_thread(action->tid)) {
        gdb_error_with_code_str(GDB_EUNIMPL,
                                "vCont: non-current thread execution unsupported");
        return false;
    }

    action->has_thread = true;
    return true;
}

/*
   Handle the action list from a 'vCont;...' packet.

   This all-stop stub accepts at most one continue or single-step action per
   packet. Optional thread qualifiers are honored only when they resolve to
   the currently stopped thread (or the equivalent any/all selectors), so the
   stub does not pretend to support scheduler-aware multi-thread resume.
*/
static bool handle_vcont_actions(char *ptr) {
    vcont_action_t selected = { 0 };
    bool have_selected = false;
    int action_count = 0;
    char *action;

    action = ptr;

    while(action) {
        vcont_action_t parsed;
        char *next = strchr(action, ';');

        if(next)
            *next++ = '\0';

        if(!parse_vcont_action(action, &parsed))
            return false;

        if(++action_count > 1) {
            gdb_error_with_code_str(GDB_EUNIMPL,
                                    "vCont: multiple actions unsupported");
            return false;
        }

        selected = parsed;
        have_selected = true;
        action = next;
    }

    if(!have_selected) {
        gdb_error_with_code_str(GDB_EINVAL, "vCont: missing action");
        return false;
    }

    return gdb_resume_target(selected.type == 's', false, 0);
}

/*
   Handle supported extended 'v' packets.

   This dispatcher implements vCont, vMustReplyEmpty, vCtrlC, and vKill[;pid].
   Unsupported optional 'v' packets fall back to an empty reply, which is the
   normal RSP way to say that an extension is not implemented. Packets that
   reboot or kill the target do not return to the normal packet-processing
   loop.
*/
bool handle_v_packet(char *ptr) {
    if(strcmp(ptr, "Cont?") == 0) {
        gdb_put_str("vCont;c;s");
        return false;
    }

    if(strncmp(ptr, "Cont;", 5) == 0)
        return handle_vcont_actions(ptr + 5);

    if(strcmp(ptr, "CtrlC") == 0) {
        put_packet(GDB_OK);
        arch_reboot();
    }

    if(strncmp(ptr, "Kill", 4) == 0 && (ptr[4] == '\0' || ptr[4] == ';')) {
        handle_kill();
        return true;
    }

    if(strcmp(ptr, "MustReplyEmpty") == 0) {
        gdb_clear_out_buffer();
        return false;
    }

    gdb_clear_out_buffer();
    return false;
}
