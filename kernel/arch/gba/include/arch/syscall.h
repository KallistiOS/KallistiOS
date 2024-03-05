/* KallistiOS 2.0.0

   arch/gba/include/syscall.h
   (c)2000-2001 Dan Potter

*/

#ifndef __ARCH_SYSCALL_H
#define __ARCH_SYSCALL_H

#include <arch/types.h>
#include <arch/irq.h>

/* Set the return value for a syscall for a dormant thread */
void syscall_set_return(irq_context_t *context, int value);

/* Handles all the nasty work */
#define SET_RETURN(thread, value) do { } while(0)

/* Ditto */
#define SET_MY_RETURN(value) do { } while(0)

/* Ditto */
#define RETURN(value) do { } while(0)

/* This macro can be used in normal mode to jump into kernel
   mode convienently */
#define SYSCALL(routine, p1, p2, p3, p4) 0

#endif  /* __ARCH_SYSCALL_H */



