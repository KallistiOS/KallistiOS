/* KallistiOS 2.0.0

   dbgio.c
   (c)2000-2001 Dan Potter
*/

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <arch/arch.h>
#include <arch/dbgio.h>


/*

Unfortunately the GBA has no standard form of serial interaction currently.
So instead we provide these stubs to allow the rest of the KOS code to
link properly.

*/

/* Enable serial support */
void dbgio_enable() { }

/* Disable serial support */
void dbgio_disable() { }


/* Initialize the SCIF port; baud_rate must be at least 9600 and
   no more than 57600. 115200 does NOT work for most PCs. */
void dbgio_init() {
}

/* Write one char to the serial port (call serial_flush()!)*/
void dbgio_write(int c) {
}

/* Flush all FIFO'd bytes out of the serial port buffer */
void dbgio_flush() {
}

/* Send an entire buffer */
void dbgio_write_buffer(uint8 *data, int len) {
}

/* Send an entire buffer */
void dbgio_write_buffer_xlat(uint8 *data, int len) {
}

/* Send a string (null-terminated) */
void dbgio_write_str(char *str) {
}

/* Read one char from the serial port (-1 if nothing to read) */
int dbgio_read() {
    return -1;
}

/* Read an entire buffer (block) */
void dbgio_read_buffer(uint8 *data, int len) {
}

int dbgio_printf(const char *fmt, ...) {
    return 0;
}

void dbgio_printk(const char *buf) {
}

