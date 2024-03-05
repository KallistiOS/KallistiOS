/* KallistiOS 2.0.0

   arch/gba/include/dbgio.h
   (c)2000 Dan Potter

*/

#ifndef __ARCH_DBGIO_H
#define __ARCH_DBGIO_H

#include <arch/types.h>

/* Initialize the SCIF port */
void dbgio_init();

/* Write one char to the serial port (call serial_flush()!)*/
void dbgio_write(int c);

/* Flush all FIFO'd bytes out of the serial port buffer */
void dbgio_flush();

/* Send an entire buffer */
void dbgio_write_buffer(uint8 *data, int len);

/* Read an entire buffer (block) */
void dbgio_read_buffer(uint8 *data, int len);

/* Send a string (null-terminated) */
void dbgio_write_str(char *str);

/* Read one char from the serial port (-1 if nothing to read) */
int dbgio_read();

/* Enable / Disable debug I/O globally */
void dbgio_disable();
void dbgio_enable();

/* Printf functionality */
int dbgio_printf(const char *fmt, ...);

#endif  /* __ARCH_DBGIO_H */

