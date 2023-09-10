/* KallistiOS ##version##

   atomics.c

   Copyright (C) 2023 Falco Girgis

   This file serves as both an example of and and a validation test for
   C11 atomics support with the SH4 DC toolchain and KOS.

   C11 atomics are an extremely convenient, easy-to-use concurrency 
   primitive supported at the language-level. They allow for thread-safe
   access to and manipulation of variables without requiring an external
   mutex or synchronization primitive to prevent multiple threads from
   trying to modify the data simultaneously.

   Atomics are also more efficient spatially, because there is no extra memory used 
   for such additional mutexes to confer thread-safety around such variables. In 
   terms of runtime, they are implemented similarly to mutexes, where interrupts are
   disabled around load/store/fetch operations.

   Most of the back-end for atomics is provided by the compiler when using the 
   "-matomic-model=soft-imask" flag; however, KOS has to implement some of the 
   back-end for primitive types (64-bit types in particular) and generic structs.

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <threads.h>
#include <stdatomic.h>

/* Test that the standard library advertises that we even have atomics. */
#ifdef __STDC_NO_ATOMICS__
#  error "The standard lib claims we don't support atomics!"
#endif

/* Test our compile-time macros for sanity. These can be used to detect
   atomic locking characteristics at compile-time for your platform. 
      0: The atomic type is never lock-free
      1: the atomic type is sometimes lock-free
      2: The atomic type is always lock-free
*/
_Static_assert(ATOMIC_BOOL_LOCK_FREE == 1,
               "Our booleans are expected to sometimes be lock free!");
_Static_assert(ATOMIC_CHAR_LOCK_FREE == 1,
               "Our chars are expected to be lock free!");
_Static_assert(ATOMIC_SHORT_LOCK_FREE == 1,
               "Our shorts are expected to be lock free!");
_Static_assert(ATOMIC_INT_LOCK_FREE == 1,
               "Our ints are expected to be lock free!");
_Static_assert(ATOMIC_LONG_LOCK_FREE == 1,
               "Our longs are expected to be lock free!");
_Static_assert(ATOMIC_LLONG_LOCK_FREE == 1,
               "Our long longs are expected to be lock free!");
_Static_assert(ATOMIC_POINTER_LOCK_FREE == 1,
               "Our pointers are expected to be lock free!");

#define THREAD_COUNT    20 /* # of threads to spawn */
#define ITERATION_COUNT 30 /* # of times to iterate over each atomic */

typedef struct {
   char values[100];
} Buffer;

/* Atomic data our threads will be competing to access. */
static atomic_flag      flag_atomic     = ATOMIC_FLAG_INIT;
static atomic_bool      bool_atomic     = false;
static atomic_int       int_atomic      = 0;
static _Atomic uint64_t longlong_atomic = 0;
static _Atomic(uint8_t) byte_atomic     = 0;
static atomic_short     short_atomic    = 0;
static atomic_ptrdiff_t ptrdiff_atomic  = 0;
static _Atomic(Buffer)  buffer_atomic   = { {0} };

//atomic_is_lock_free()

/* Per-thread logic */
int thread(void *arg) { 

   return 0;

}

int main(int arg, char* argv[]) { 
   int retval = 0;
   thrd_t threads[THREAD_COUNT - 1];

   printf("Checking locking characteristics.\n");

   if(atomic_is_lock_free(&bool_atomic)) {
      fprintf(stderr, "Bool atomics are lock free!\n");
      retval = -1;
   }

   if(atomic_is_lock_free(&byte_atomic)) {
      fprintf(stderr, "Byte atomics are lock free!\n");
      retval = -1;
   }

   if(atomic_is_lock_free(&short_atomic)) {
      fprintf(stderr, "Short atomics are lock free!\n");
      retval = -1;
   }

   if(atomic_is_lock_free(&int_atomic)) {
      fprintf(stderr, "Int atomics are lock free!\n");
      retval = -1;
   }

   if(atomic_is_lock_free(&longlong_atomic)) {
      fprintf(stderr, "Long long atomics are lock free!\n");
      retval = -1;
   }

   if(atomic_is_lock_free(&ptrdiff_atomic)) {
      fprintf(stderr, "Ptrdiff atomics are lock free!\n");
      retval = -1;
   }

   if(atomic_is_lock_free(&buffer_atomic)) {
      fprintf(stderr, "Struct atomics are lock free!\n");
      retval = -1;
   }

   printf("Running threads: [%u]\n", THREAD_COUNT);

   for(unsigned t = 0; t < THREAD_COUNT - 1; ++t) {
      if(thrd_create(&threads[t], thread, (void *)t + 1)
            != thrd_success) 
      {
         fprintf(stderr, "Failed to create thread: [%u]\n", t + 1);
         retval = -1;
      }
   }

   /* Run the same logic for the main thread. */
   if(thread((void *)0) == -1) 
      retval = -1;

   printf("Joining threads: [%u]\n", THREAD_COUNT);
   
   for(unsigned t = 0; t < THREAD_COUNT - 1; ++t) {
      int res;
      if(thrd_join(threads[t], &res) == thrd_error) {
         fprintf(stderr, "Failed to join thread: [%u]\n", t + 1);
         retval = -1;
         continue;
      }

      if(res == -1) 
         retval = -1;
   }   

   if(retval == -1) {
      fprintf(stderr, "\n\nATOMICS TEST FAILED!!!\n\n");
      return EXIT_FAILURE;
   }
   else {
      printf("\n\nATOMICS TEST PASSED!!!\n\n");
      return EXIT_SUCCESS;
   }
}