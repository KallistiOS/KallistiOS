#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include <kos/irq.h>
#include <kos/thread.h>

/*
 * BUFMAX defines the maximum number of characters in inbound/outbound
 * buffers. At least NUMREGBYTES*2 are needed for register packets.
 */
#define BUFMAX         1024

#define GDB_OK         "OK"
#define GDB_THREAD_ALL (-1)
#define GDB_THREAD_ANY 0
#define GDB_EINVAL     "E01"
#define GDB_EUNIMPL    "E02"
#define GDB_EBADCMD    "E03"
#define GDB_EMEM_SIZE  "E34"
#define GDB_EMEM_PROT  "E35"
#define GDB_EBKPT_SET_FAIL   "E50"
#define GDB_EBKPT_HW_NORES   "E52"
#define GDB_EBKPT_HW_ACCESS  "E53"
#define GDB_EBKPT_CLEAR_ID   "E55"
#define GDB_EBKPT_SW_NORES   "E57"
#define GDB_EBKPT_CLEAR_ADDR "E56"

char highhex(int x);
char lowhex(int x);
int hex(char ch);
int format_thread_id_hex(char out[9], uint32_t tid);

char *mem_to_hex(const char *src, char *dest, size_t count);
char *hex_to_mem(const char *src, char *dest, size_t count);
size_t hex_to_int(char **ptr, uint32_t *int_value);
void undo_single_step(void);
void gdb_enter_exception(irq_context_t *context, int exception_vector, bool rewind_pc);
irq_context_t *gdb_get_irq_context(void);
irq_context_t *gdb_resolve_thread_context(int tid);
void gdb_set_connected(bool is_connected);

void set_regs_thread(int tid);
void set_ctrl_thread(int tid);
void setup_regs_context(void);
void setup_ctrl_context(void);
bool gdb_resume_target(bool stepping, bool set_pc, uint32_t pc);
char *append_regs(char *out, const irq_context_t *context);
void gdb_put_ok(void);
void gdb_put_str(const char *msg);
void gdb_clear_out_buffer(void);
char *gdb_get_out_buffer(void);
size_t gdb_get_in_packet_length(void);
void set_error_messages_enabled(bool enabled);
void set_no_ack_mode_enabled(bool enabled);
void gdb_error_with_code_str(const char *errcode, const char *msg_fmt, ...);
void gdb_set_connected(bool is_connected);

extern int using_dcl;
extern char remcom_out_buffer[];
extern irq_context_t *irq_ctx;

unsigned char *get_packet(void);
void put_packet(const char *buffer);

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
void handle_thread_select(char *ptr);
void handle_detach(void);
void handle_kill(void);
void handle_t_stop_reply(int exception_vector);
bool handle_v_packet(char *ptr);
