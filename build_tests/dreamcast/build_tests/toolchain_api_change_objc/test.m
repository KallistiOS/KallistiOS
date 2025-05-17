/* KallistiOS ##version##

   test.c
   Copyright (C) 2025 Donald Haase

   This is intended to validate that the API required by the toolchain
   to build correctly hasn't changed.
*/

#import <objc/objc.h>
#import <objc/Object.h>
#import <objc/runtime.h>
#import <objc/thr.h>

#include <stdlib.h>

/* Ensure we're testing the libobj patch */
#ifndef _LIBOBJC
    #define _LIBOBJC
#endif

/* Include the patch header to see if there are conflicts */
#include <gthr-kos.h>

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    return EXIT_SUCCESS;
}
