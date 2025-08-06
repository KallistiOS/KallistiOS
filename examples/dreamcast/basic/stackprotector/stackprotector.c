/* KallistiOS ##version##

   stackprotector.c
   Copyright (C) 2021 Lawrence Sebald
   Copyright (C) 2024 Falco Girgis
*/

/* This example shows how to make use of GCC's -fstack-protector options to
   enable stack-smashing protection in your code. You must add a
   -fstack-protector option to your KOS_CFLAGS in your Makefile to make use
   of this functionality (this example uses -fstack-protector-all).

   This example also shows how to override the default stack protector
   failure handler to customize it (we add the PR address to the output to
   help narrow down where the error occurred.

   Keep in mind that optimizations may shift things around, so for best
   results you should be using the stack protector with optimizations
   disabled. Here, we manually disable inlining on the function that trips
   the stack protector to ensure that it trips in the correct function.

   Thanks to mrneo240 for the idea of printing out the PR in the handler and
   for giving me the impetus to actually add the ability to override the
   handler to the Newlib patch.
*/

#include <string.h>
#include <stdio.h>
#include <arch/stack.h>
#include <stdlib.h>

/* This function will override the default stack protector handler that is
   defined in Newlib. This is not necessary to enable the stack protector,
   but is nice for being able to draw the error message to the screen or
   whatnot (not that we do any of that here).

   NOTE: Typically you would want to call abort() and/or write to stderr
         rather than stdout, but we exit gracefully in this example due to
         the fact that we're simply testing the stack checker functionality.

   WARNING: This function must be explicitly marked with the used attribute
            when compiling with LTO enabled to prevent it from being removed,
            which means you will be calling into Newlib's default handler.
*/
__used void __stack_chk_fail(void) {
    unsigned int pr = (unsigned int)arch_get_ret_addr();
    printf("Stack smashed at PR=0x%08x\n", pr);
    printf("Successfully detected stack corruption!\n");
    exit(EXIT_SUCCESS);
}

/* Make sure the compiler doesn't complain about the bad thing 
we are doing intentionally */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-out-of-bounds"

__noinline void badfunc(void) {
    char buffer[8];
    strcpy(buffer, "This string is entirely too long and will overflow.");
}

/* Turn the warning back on */
#pragma GCC diagnostic pop

int main(int argc, char **argv) {
    printf("Stack protector test....\n");
    printf("About to call badfunc()...\n");

    badfunc();

    fprintf(stderr, "This shouldn't print out if stack protector is enabled.\n");

    return EXIT_FAILURE;
}


