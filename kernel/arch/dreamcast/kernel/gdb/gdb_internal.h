#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include <kos/irq.h>

/*
 * BUFMAX defines the maximum number of characters in inbound/outbound
 * buffers. At least NUMREGBYTES*2 are needed for register packets.
 */
#define BUFMAX         1024

#define GDB_OK         "OK"

char highhex(int x);
char lowhex(int x);
int hex(char ch);

char *mem_to_hex(const char *src, char *dest, size_t count);
char *hex_to_mem(const char *src, char *dest, size_t count);
size_t hex_to_int(char **ptr, uint32_t *int_value);
void undo_single_step(void);

extern int using_dcl;
extern char remcom_out_buffer[];
extern irq_context_t *irq_ctx;

unsigned char *get_packet(void);
void put_packet(const char *buffer);

void handle_read_regs(char *ptr);
void handle_write_regs(char *ptr);
void handle_read_mem(char *ptr);
void handle_write_mem(char *ptr);
void handle_continue_step(char *ptr);
void handle_breakpoint(char *ptr);
