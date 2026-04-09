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

void set_regs_thread(int tid);
void set_ctrl_thread(int tid);
void setup_regs_context(void);
void setup_ctrl_context(void);

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
bool handle_continue_step(char *ptr);
void handle_breakpoint(char *ptr);
void handle_query(char *ptr);
void handle_thread_alive(char *ptr);
void handle_thread_select(char *ptr);
