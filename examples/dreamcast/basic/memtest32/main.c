/* KallistiOS ##version##

   main.c
   Copyright (C) 2020 Thomas Sowell
   Copyright (C) 2022 Eric Fradella

   This application illustrates the use of functions related to detecting the
   size of system memory and altering a program's behavior to suit the running
   system's configuration.

   Implemented is a memory test utility for Dreamcast consoles (both stock 16MB
   systems and modified 32MB systems) or NAOMI systems, based on public domain
   code by Michael Barr in memtest.c found here:
   https://barrgroup.com/embedded-systems/how-to/memory-test-suite-c

   Example output on a functional 32MB-modified Dreamcast:

   Beginning memtest routine...
    Base address: 0x8c100000
    Number of bytes to test: 32440320
     memTestDataBus: PASS
     memTestAddressBus: PASS
     memTestDevice: PASS
   Test passed!
*/

#include <stdio.h>
#include <stdint.h>

#include <kos/init.h>
#include <arch/arch.h>

#include "memtest.h"

/* Leave space at the beginning and end of memory for this program and for the
 * stack. Dreamcast applications are loaded at 0x8c000000 so we leave 0x100000
 * bytes past that for the program itself and leave 65536 bytes at the top of
 * memory for the stack. */
#define SAFE_AREA     0x100000
#define STACK_SIZE    65536
#define BASE_ADDRESS  (volatile datum *) (0x8c000000 + SAFE_AREA)

/* Define the number of bytes to be tested. KallistiOS provides the
 * macros HW_MEM_16 and HW_MEM_32 in <arch/arch.h> which describe
 * the number of bytes available in standard supported console
 * configurations (16777216 and 33554432, respectively). */
#define NUM_BYTES_32  (HW_MEM_32 - SAFE_AREA - STACK_SIZE)
#define NUM_BYTES_16  (HW_MEM_16 - SAFE_AREA - STACK_SIZE)

int main(int argc, char **argv) {
    uint32_t error, data, *address;
    unsigned long num_bytes;
    error = 0;

    /* The HW_MEMSIZE can be called to retrieve the system's memory size.
     * _arch_mem_top defines the top address of memory.
     * 0x8d000000 if 16MB console, 0x8e000000 if 32MB */
    printf("\nThis console has %ld bytes of system memory,\n with top of "
           "memory located at 0x%0lx.\n\n", HW_MEMSIZE, _arch_mem_top);

    /* The DBL_MEM boolean macro is provided as an easy, concise
     * way to determine if extra system RAM is available */
    num_bytes = DBL_MEM ? NUM_BYTES_32 : NUM_BYTES_16;

    printf("Beginning memtest routine...\n");
    printf(" Base address: %p\n", BASE_ADDRESS);
    printf(" Number of bytes to test: %lu\n", num_bytes);

    /* Now we run the test routines provided in memtest.c
     * Each routine returns zero if the routine passes,
     * else it returns the address of failure.
     * First, let's test the data bus. */
    printf("  memTestDataBus: ");
    fflush(stdout);
    data = memTestDataBus(BASE_ADDRESS);

    if(data != 0) {
        printf("FAIL: %08lx\n", data);
        error = 1;
    }
    else {
        printf("PASS\n");
    }

    fflush(stdout);

    /* Now we test the address bus. */
    printf("  memTestAddressBus: ");
    fflush(stdout);
    address = memTestAddressBus(BASE_ADDRESS, num_bytes);

    if(address != NULL) {
        printf("FAIL (%p)\n", address);
        error = 1;
    }
    else {
        printf("PASS\n");
    }

    fflush(stdout);

    /* And now, we test the memory itself. */
    printf("  memTestDevice: ");
    fflush(stdout);
    address = memTestDevice(BASE_ADDRESS, num_bytes);

    if(address != NULL) {
        printf("FAIL (%p)\n", address);
        error = 1;
    }
    else {
        printf("PASS\n");
    }

    fflush(stdout);

    /* Test completed, return final result */
    printf("Test %s\n", error ? "failed." : "passed!\n");
    return error;
}
