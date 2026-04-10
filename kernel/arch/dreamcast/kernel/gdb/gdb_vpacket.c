/* KallistiOS ##version##

   kernel/gdb/gdb_vpacket.c

   Copyright (C) 2026 Andy Barajas

*/

#include <arch/arch.h>

#include "gdb_internal.h"

static bool handle_vcont(char *ptr) {
    if(strncmp(ptr, "Cont?", 5) == 0) {
        gdb_put_str("vCont;c;s");
        return false;
    }

    if(strcmp(ptr, "Cont;c") == 0) {
        char command[] = "c";
        handle_continue_step(command + 1);
        return true;
    }

    if(strcmp(ptr, "Cont;s") == 0) {
        char command[] = "s";
        handle_continue_step(command + 1);
        return true;
    }

    gdb_error_with_code_str(GDB_EUNIMPL, "vCont: unsupported action");
    return false;
}

bool handle_v_packet(char *ptr) {
    if(strncmp(ptr, "Cont", 4) == 0)
        return handle_vcont(ptr);

    if(strcmp(ptr, "CtrlC") == 0) {
        put_packet(GDB_OK);
        arch_reboot();
    }

    if(strcmp(ptr, "Kill") == 0)
        handle_kill();

    if(strncmp(ptr, "MustReplyEmpty", 14) == 0) {
        gdb_clear_out_buffer();
        return false;
    }

    gdb_clear_out_buffer();
    return false;
}
