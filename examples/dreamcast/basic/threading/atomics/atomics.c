/* KallistiOS ##version##

   examples/dreamcast/basic/threading/atomics.c

   Copyright (C) 2023 Falco Girgis

   This file serves as both an example of and a validation test for
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
#include <limits.h>
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
_Static_assert(ATOMIC_BOOL_LOCK_FREE == 2,
               "Our booleans are expected to be lock free!");
_Static_assert(ATOMIC_CHAR_LOCK_FREE == 2,
               "Our chars are expected to be lock free!");
_Static_assert(ATOMIC_SHORT_LOCK_FREE == 2,
               "Our shorts are expected to be lock free!");
_Static_assert(ATOMIC_INT_LOCK_FREE == 2,
               "Our ints are expected to be lock free!");
_Static_assert(ATOMIC_LONG_LOCK_FREE == 2,
               "Our longs are expected to be lock free!");
_Static_assert(ATOMIC_POINTER_LOCK_FREE == 2,
               "Our pointers are expected to be lock free!");
_Static_assert(ATOMIC_LLONG_LOCK_FREE == 1,
               "Our long longs are expected to sometimes be lock free!");

#define THREAD_COUNT    20 /* # of threads to spawn */
#define ITERATION_COUNT 5 /* # of times to iterate over each atomic */

typedef struct {
   char values[100];
} Buffer;

/* Atomic data our threads will be competing to access. */
static atomic_flag      flag_atomic     = ATOMIC_FLAG_INIT;
static atomic_bool      bool_atomic     = false;
static atomic_int       int_atomic      = INT_MAX;
static _Atomic uint64_t longlong_atomic = 0;
static _Atomic(uint8_t) byte_atomic     = 0;
static atomic_short     short_atomic    = 0;
static atomic_ptrdiff_t ptrdiff_atomic  = 0;
static _Atomic(Buffer)  buffer_atomic   = { {0} };

static void atomic_increment_buffer(void) {
   Buffer buff = atomic_load(&buffer_atomic);

   for(unsigned v = 0; v < sizeof(buff.values); ++v)
      buff.values[v]++;

   atomic_store(&buffer_atomic, buff);
}

/* Per-thread logic */
int thread(void *arg) { 
   unsigned tid = (unsigned)arg;
   int retval = 0;

   for(unsigned i = 0; i < ITERATION_COUNT; ++i) {

      while(atomic_flag_test_and_set(&flag_atomic))
         printf("[Thread: %u]: Waiting to acquire flag_atomic.\n", tid);

      printf("[Thread: %u]: Acquired flag_atomic.\n", tid);

      thrd_yield();
      atomic_fetch_add(&longlong_atomic, 1);

      atomic_flag_clear(&flag_atomic);
      printf("[Thread: %u]: Released flag_atomic.\n", tid);

      bool expected = false;
      while(!atomic_compare_exchange_weak(&bool_atomic,
                                          &expected,
                                          true))
      { 
         expected = false;     
      }
      printf("[Thread: %u]: Acquired bool_atomic.\n", tid);

      static const struct timespec time = { .tv_nsec = 100000000 };
      thrd_sleep(&time, NULL);

      atomic_fetch_sub_explicit(&short_atomic,
                                1, 
                                memory_order_relaxed);

      expected = true;
      if(!atomic_compare_exchange_strong(&bool_atomic,
                                         &expected,
                                         false))
      {
         fprintf(stderr, 
                 "[Thread: %u]: Unexpected value for bool atomic!\n",
                 tid);
         retval = -1;
      }
      printf("[Thread: %u]: Released bool_atomic.\n", tid);

      atomic_fetch_or(&byte_atomic, tid);

      atomic_fetch_xor(&ptrdiff_atomic, tid);

      atomic_fetch_and(&int_atomic, tid);

      atomic_increment_buffer();
   }

   return retval;
}

int main(int arg, char* argv[]) { 
   int retval = 0;
   thrd_t threads[THREAD_COUNT - 1];

   printf("Checking locking characteristics.\n");

   if(!atomic_is_lock_free(&bool_atomic)) {
      fprintf(stderr, "Bool atomics are not lock free!\n");
      retval = -1;
   }

   if(!atomic_is_lock_free(&byte_atomic)) {
      fprintf(stderr, "Byte atomics are not lock free!\n");
      retval = -1;
   }

   if(!atomic_is_lock_free(&short_atomic)) {
      fprintf(stderr, "Short atomics are not lock free!\n");
      retval = -1;
   }

   if(!atomic_is_lock_free(&int_atomic)) {
      fprintf(stderr, "Int atomics are not lock free!\n");
      retval = -1;
   }

   if(!atomic_is_lock_free(&longlong_atomic)) {
      fprintf(stderr, "Long long atomics are not lock free!\n");
      retval = -1;
   }

   if(!atomic_is_lock_free(&ptrdiff_atomic)) {
      fprintf(stderr, "Ptrdiff atomics are not lock free!\n");
      retval = -1;
   }

   if(!atomic_is_lock_free(&buffer_atomic)) {
      fprintf(stderr, "Struct atomics are not lock free!\n");
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

   printf("Validating results.\n");

   /* Verify atomic_flag state. */
   if(atomic_flag_test_and_set(&flag_atomic)) {
      fprintf(stderr, "flat_atomic left in unexpected state: [true]\n");
      retval = -1;
   }

   /* Verify atomic bool state. */
   bool expected = false;
   if(!atomic_compare_exchange_weak(&bool_atomic,
                                    &expected,
                                    true))
   {
      fprintf(stderr, "bool_atomic left in unexpected state: [true]\n");
      retval = -1;
   }

   /* Verify atomic byte state. */
   uint8_t byte_value, byte_expected_value = 0;
   for(unsigned i = 0; i < THREAD_COUNT; ++i)
      byte_expected_value |= i;
   if((byte_value = atomic_load_explicit(&byte_atomic, memory_order_acquire))
         != byte_expected_value)
   {
      fprintf(stderr, 
              "byte_atomic left in unexpected state: [%u]\n", 
              byte_value);
      retval = -1;
   }

   /* Verify atomic long long state. */
   uint64_t longlong_value;
   if((longlong_value = atomic_load(&longlong_atomic)) 
         != THREAD_COUNT * ITERATION_COUNT) 
   {
      fprintf(stderr, 
              "longlong_atomic left in unexpected state: [%llu]\n", 
              longlong_value);
      retval = -1;
   }

   /* Verify atomic short state. */
   short short_value;
   if((short_value = atomic_load_explicit(&short_atomic, memory_order_consume))
         != -(THREAD_COUNT * ITERATION_COUNT))
   {
      fprintf(stderr, 
              "short_atomic left in unexpected state: [%i]\n", 
              short_value);
      retval = -1;
   }

   /* Verify atomic int state. */
   int int_value, int_expected_value = INT_MAX;
   for(int i = 0; i < THREAD_COUNT; ++i)
      int_expected_value &= i;
   if((int_value = atomic_load(&int_atomic)) 
         != int_expected_value)
   {
      fprintf(stderr, 
              "int_atomic left in unexpected state: [%d]\n", 
              int_value);
      retval = -1;
   }

   /* Verify atomic ptrdiff_t state. */
   ptrdiff_t ptrdiff_value, ptrdiff_expected_value = 0;
   for(unsigned i = 0; i < THREAD_COUNT; ++i)
      ptrdiff_expected_value ^= i;
   if((ptrdiff_value = atomic_load(&ptrdiff_atomic)) 
         != ptrdiff_expected_value)
   {
      fprintf(stderr, 
              "ptrdiff_atomic left in unexpected state: [%d]\n", 
              ptrdiff_value);
      retval = -1;
   }

   /* Verify atomic buffer state. */
   Buffer buff_value = atomic_load(&buffer_atomic);
   for(unsigned v = 0; v < sizeof(buff_value.values); ++v) {
      if(buff_value.values[v] != THREAD_COUNT * ITERATION_COUNT) {
         fprintf(stderr, 
                 "buffer_atomic[%u] left in unexpected state: [%d]",
                 v, 
                 buff_value.values[v]);
         retval = -1;
      }
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

