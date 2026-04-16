/* KallistiOS ##version##

   arch/dreamcast/kernel/gdb/gdb_internal.h

   Copyright (C) 2026 Andy Barajas

*/

/*
   This header defines internal structures, macros, and utility functions used
   throughout the GDB remote stub implementation. It includes definitions for
   communication buffers, error codes, register/context handling, and packet
   management. These interfaces are intended for use only within the GDB stub
   implementation and are not part of the public API.
*/

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <string.h>

#include <kos/irq.h>
#include <kos/thread.h>

extern int using_dcl;
extern char remcom_out_buffer[];

/* Total inbound/outbound packet-buffer capacity, including framing workspace. */
#define BUFMAX            1024

#define GDB_OK            "OK"
#define GDB_THREAD_ALL    -1
#define GDB_THREAD_ANY    0

/* General Errors (E00–E0F) */
#define GDB_EINVAL             "E01"  /* Invalid argument or state */
#define GDB_EUNIMPL            "E02"  /* Unimplemented command/feature */
#define GDB_EBADCMD            "E03"  /* Unknown or unsupported command */
#define GDB_ETIMEOUT           "E04"  /* General communication timeout */
#define GDB_EGENERIC           "E05"  /* Unclassified internal error */

/* Memory Errors (E30–E3F) */
#define GDB_EMEM_ADDR          "E30"  /* Invalid or unmapped address */
#define GDB_EMEM_BUS           "E31"  /* Bus error (e.g. MMU fault) */
#define GDB_EMEM_TIMEOUT       "E32"  /* Timeout accessing memory */
#define GDB_EMEM_VERIFY        "E33"  /* Memory verification failed */
#define GDB_EMEM_SIZE          "E34"  /* Misaligned or invalid access size */
#define GDB_EMEM_PROT          "E35"  /* Invalid or prohibited memory access */
#define GDB_EMEM_UNKNOWN       "E3F"  /* Generic/unspecified memory fault */

/* Breakpoint/Watchpoint Errors (E50–E5F) */
#define GDB_EBKPT_SET_FAIL     "E50"  /* General failure setting breakpoint */
#define GDB_EBKPT_SW_OVERWRITE "E51"  /* Couldn't overwrite memory for sw bkpt */
#define GDB_EBKPT_HW_NORES     "E52"  /* No UBC resource available */
#define GDB_EBKPT_HW_ACCESS    "E53"  /* Failed accessing UBC registers */
#define GDB_EBKPT_CLEAR_ID     "E55"  /* Invalid breakpoint ID on clear */
#define GDB_EBKPT_CLEAR_ADDR   "E56"  /* Address doesn't match any active bkpt */
#define GDB_EBKPT_SW_NORES     "E57"  /* Too many software breakpoints */

/* Reserved Ranges for Future Use */
/*
   E1x: Threading or process management (not currently implemented)
   E2x: Filesystem or peripheral I/O errors
   E4x: FPU/vector/pseudo-register errors
   E6x: Watchpoints, memory monitoring
   E7x: User-defined stub extensions
*/

/* Utilities (gdb_utils.c) */
int hex(char ch);
char highhex(int x);
char lowhex(int x);
size_t hex_to_int(char **ptr, uint32_t *int_value);
char *mem_to_hex(const char *src, char *dest, size_t count);
char *hex_to_mem(const char *src, char *dest, size_t count);

/* Register/Control (gdb_regs.c/gdb_ctrl.c) */
void set_regs_thread(int tid);
void set_ctrl_thread(int tid);
void setup_regs_context(void);
void setup_ctrl_context(void);
bool gdb_resume_target(bool stepping, bool set_pc, uint32_t pc);

/* Packet (gdb_packet.c) */
void set_error_messages_enabled(bool enabled);
void set_no_ack_mode_enabled(bool enabled);
void gdb_error_with_code_str(const char *errcode, const char *msg_fmt, ...);
void gdb_put_ok(void);
void gdb_put_str(const char *msg);
void gdb_clear_out_buffer(void);
char *gdb_get_out_buffer(void);
size_t gdb_get_in_packet_length(void);
unsigned char *get_packet(void);
void put_packet(const char *buffer);

/* Misc */
irq_context_t *gdb_get_irq_context(void);
void gdb_enter_exception(irq_context_t *context, int exception_vector, bool rewind_pc);
irq_context_t *gdb_resolve_thread_context(int tid);
void gdb_set_connected(bool is_connected);
void undo_single_step(void);
int format_thread_id_hex(char out[9], uint32_t tid);
char *append_regs(char *out, size_t *remaining, const irq_context_t *context);

/* Handlers */
void handle_detach(void);
void handle_kill(void);
void handle_t_stop_reply(int exception_vector);
void handle_thread_select(char *ptr);
void handle_read_reg(char *ptr);
void handle_write_reg(char *ptr);
void handle_read_regs(char *ptr);
void handle_write_regs(char *ptr);
void handle_read_mem(char *ptr);
void handle_write_mem(char *ptr);
void handle_read_mem_binary(char *ptr);
void handle_write_mem_binary(char *ptr);
bool handle_continue_step(char command, char *ptr);
bool handle_continue_step_signal(char command, char *ptr);
void handle_breakpoint(char *ptr);
void handle_query(char *ptr);
void handle_set_query(char *ptr);
void handle_thread_alive(char *ptr);
bool handle_v_packet(char *ptr);
