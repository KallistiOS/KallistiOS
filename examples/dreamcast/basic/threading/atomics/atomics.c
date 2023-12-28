/* KallistiOS ##version##

   examples/dreamcast/basic/threading/atomics.c

   Copyright (C) 2023 Falco Girgis

   This file serves as both an example of and a validation test for the
   C11 atomics support provided by the SH-GCC toolchain and KOS. It 
   demonstrates advanced compiler-aware concurrency in pure standard C.

   C11 atomics are an extremely convenient, easy-to-use concurrency 
   primitive supported at the language-level. They allow for thread-safe
   access to and manipulation of variables without requiring an external
   mutex or synchronization primitive to prevent multiple threads from
   trying to modify the data simultaneously.

   Atomics are also more efficient spatially on Dreamcast, because there is 
   no extra memory used for such additional mutexes to confer thread-safety 
   around such variables. In terms of runtime, they are implemented similarly 
   to mutexes, where interrupts are disabled around load/store/fetch operations.

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

#define THREAD_COUNT          20    /* # of threads to spawn */
#define ITERATION_COUNT       5     /* # of times to iterate over each atomic */
#define BUFFER_SIZE           4096  /* # of bytes in atomic buffer */
#define BUFFER_UPDATE_COUNT   100   /* # of times to update atomic buffer */

_Static_assert(THREAD_COUNT * ITERATION_COUNT <= UINT8_MAX,
               "Threads * iterations would overflow uint8_t counters!");

/* Generic buffer structure we will use with atomics. */
typedef struct {
   uint8_t values[BUFFER_SIZE]; 
} Buffer;

/* Atomic data our threads will be competing over accessing. */
static atomic_flag      flag_atomic     = ATOMIC_FLAG_INIT;
static atomic_bool      bool_atomic     = false;
static atomic_int       int_atomic      = INT_MAX;
static _Atomic uint64_t longlong_atomic = 0;
static _Atomic(uint8_t) byte_atomic     = 0;
static atomic_short     short_atomic    = 0;
static atomic_ptrdiff_t ptrdiff_atomic  = 0;
static _Atomic(Buffer)  buffer_atomic   = { {0} };

/* Utility function to do an "atomic" fetch + add operation on our 
   massive buffer. */ 
static void atomic_add_buffer(unsigned tid, int delta) {
   /* Preload the initial value of the buffer. */
   Buffer desired, expected = atomic_load(&buffer_atomic);

   /* Attempt to update the value of our buffer to the "desired"
      value. If another thread has preempted our "atomic" operation
      and has modified the value first, atomic_compare_exchange() 
      will return false, updating the "expected" value. We increment 
      the new value and try again. */
   do {
      printf("Thread[%2u]: Attempting to add to buffer: [%u]\n", 
             tid,
             expected.values[0]);

      for(unsigned v = 0; v < sizeof(desired.values); ++v)
         desired.values[v] = expected.values[v] + delta;

   } while(!atomic_compare_exchange_strong(&buffer_atomic,
                                           &expected,
                                           desired));

   printf("Thread[%2u]: Successfully incremented buffer.\n", tid);
}

/* Per-thread entry point. */
static int thread(void *arg) { 
   unsigned tid = (unsigned)arg;
   int retval = 0;

   /* Do several iterations worth of atomic operations for each thread. */
   for(unsigned i = 0; i < ITERATION_COUNT; ++i) {
      /* Create a spin-lock out of "flag_atomic" by repeatedly testing
         and setting its value, while its previous value was not false
         (our initial "unlocked" state). */
      while(atomic_flag_test_and_set(&flag_atomic))
         printf("Thread[%2u]: Waiting to atomic flag lock.\n", tid);

      printf("Thread[%2u]: Acquired atomic flag lock.\n", tid);

      /* Yield thread execution within our "critical section" to see
         that other threads aren't able to set "flag_atomic" and enter
         it as well. */
      thrd_yield();

      /* Increment the longlong_atomic as a regular counter. */
      atomic_fetch_add(&longlong_atomic, 1);

      /* Release the ghetto flag_atomic spinlock. */
      atomic_flag_clear(&flag_atomic);
      printf("Thread[%2u]: Released atomic flag lock.\n", tid);

      /* Attempt to atomically add a value to our huge buffer structure a 
         few times, to ensure the scheduler isn't able to preempt and break
         its atomicity. */
      for(int i = 0; i < BUFFER_UPDATE_COUNT; ++i) {
         atomic_add_buffer(tid, i);
         atomic_add_buffer(tid, -i);
      }

      /* Create a second type of spinlock out of an atomic 
         boolean, doing the same thing as before: attempting
         to set "bool_atomic" to true if it was previously false
         to claim the lock. */
      bool expected = false;
      while(!atomic_compare_exchange_weak(&bool_atomic,
                                          &expected,
                                          true)) { 
         printf("Thread[%2u]: Waiting to acquire atomic bool lock.\n", tid);
         /* This time we won't hog all of the CPU time waiting. */
         thrd_yield();
         expected = false;     
      }

      printf("Thread[%2u]: Acquired atomic bool lock.\n", tid);

      /* This time lets give other threads a chance to mess
         with us by sleeping for 1ms rather than yielding. */
      static const struct timespec time = { .tv_nsec = 100000000 };
      thrd_sleep(&time, NULL);

      /* Decrement "short_atomic," treating it as a negative counter. */
      atomic_fetch_sub(&short_atomic, 1);

      printf("Thread[%2u]: Releasing atomic bool lock.\n", tid);
      
      /* Do the reverse compare + exchange operation to release the lock, this
         time expecting it to be previously owned (true) and setting it to 
         false to release. */
      expected = true;
      if(!atomic_compare_exchange_strong(&bool_atomic,
                                         &expected,
                                         false)) {
         fprintf(stderr, 
                 "Thread[%2u]: Unexpected value for atomic bool lock!\n",
                 tid);
         retval = -1;
      }

      /* Ensure atomic bitwise operations work on various primitive types. */
      printf("Thread[%2u]: Performing bitwise operations.\n", tid);

      atomic_fetch_or(&byte_atomic, tid);
      atomic_fetch_xor(&ptrdiff_atomic, tid);
      atomic_fetch_and(&int_atomic, tid);
   }

   /* Return any errors back to the main thread. */
   return retval;
}

/* Main thread entry point. */
int main(int arg, char* argv[]) { 
   int retval = 0;
    /* Array containing our spawned threads. */
   thrd_t threads[THREAD_COUNT - 1]; 

   /* First lets ensure that our atomics report the proper
      runtime values for being lock-free. */
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

   if(atomic_is_lock_free(&buffer_atomic)) {
      fprintf(stderr, "Struct atomics are lock free!\n");
      retval = -1;
   }

   printf("Running threads: [%u]\n", THREAD_COUNT);

   /* Create N-1 threads running the "thread()" function to
      manipulate and fight over various atomic data. */ 
   for(unsigned t = 0; t < THREAD_COUNT - 1; ++t) {
      if(thrd_create(&threads[t], thread,
                     (void *)t + 1) != thrd_success) {
         fprintf(stderr, "Failed to create thread: [%u]\n", t + 1);
         retval = -1;
      }
   }

   /* Run the same logic for the main thread as the last one (Thread 0). */
   if(thread((void *)0) == -1) 
      retval = -1;

   /* Await for each thread to complete its tasks and return its
      status code. */
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

   /* Validate that all of our atomics have their expected final values. */
   printf("\nValidating results:\n");

   /* Verify atomic_flag state. */
   if(atomic_flag_test_and_set(&flag_atomic)) {
      fprintf(stderr, "flag_atomic left in unexpected state: [true]\n");
      retval = -1;
   }
   else {
       printf("\tFlag atomics work.\n");
   }

   /* Verify atomic bool state. */
   bool expected = false;
   if(!atomic_compare_exchange_weak(&bool_atomic,
                                    &expected,
                                    true)) {
      fprintf(stderr, "bool_atomic left in unexpected state: [true]\n");
      retval = -1;
   }
   else {
       printf("\tBool atomics work.\n");
   }

   /* Verify atomic byte state. */
   uint8_t byte_value, byte_expected_value = 0;
   for(unsigned i = 0; i < THREAD_COUNT; ++i) {
       byte_expected_value |= i;
   }

   if((byte_value = atomic_load(&byte_atomic)) != byte_expected_value) {
      fprintf(stderr, 
              "byte_atomic left in unexpected state: [%u]\n", 
              byte_value);
      retval = -1;
   }
   else {
       printf("\t8-bit atomics work.\n");
   }

   /* Verify atomic short state. */
   short short_value;
   /* Note that memory order shouldn't do anything for our platform, just testing. */
   if((short_value = atomic_load(&short_atomic)) !=
           -(THREAD_COUNT * ITERATION_COUNT)) {
      fprintf(stderr, 
              "short_atomic left in unexpected state: [%i]\n", 
              short_value);
      retval = -1;
   }
   else {
       printf("\t16-bit atomics work.\n");
   }

   /* Verify atomic int state. */
   int int_value, int_expected_value = INT_MAX;
   for(int i = 0; i < THREAD_COUNT; ++i) {
       int_expected_value &= i;
   }
   
   if((int_value = atomic_load(&int_atomic)) != int_expected_value) {
      fprintf(stderr, 
              "int_atomic left in unexpected state: [%d]\n", 
              int_value);
      retval = -1;
   }
   else {
       printf("\t32-bit atomics work.\n");
   }

   /* Verify atomic long long state. */
   uint64_t longlong_value;
   if((longlong_value = atomic_load(&longlong_atomic)) !=
           THREAD_COUNT * ITERATION_COUNT) {
      fprintf(stderr, 
              "longlong_atomic left in unexpected state: [%llu]\n", 
              longlong_value);
      retval = -1;
   }
   else {
       printf("\t64-bit atomics work.\n");
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
   else {
       printf("\tptrdiff_t atomics work.\n");
   }

   /* Verify atomic buffer state. */
   Buffer buff_value = atomic_load(&buffer_atomic);
   bool buffer_works = true;
   for(unsigned v = 0; v < sizeof(buff_value.values); ++v) {
      if(buff_value.values[v]) {
         fprintf(stderr, 
                 "buffer_atomic[%u] left in unexpected state: [%d]\n",
                 v, 
                 buff_value.values[v]);
         buffer_works = false;
         retval = -1;
      }
   }

   if(buffer_works) 
      printf("\tGeneric atomics work.\n");

   /* Print final test results and return status code. */ 
   if(retval == -1) {
      fprintf(stderr, "\n***** C11 ATOMICS TEST FAILED! *****\n\n");
      return EXIT_FAILURE;
   }
   else {
      printf("\n***** C11 ATOMICS TEST PASSED! *****\n\n");
      return EXIT_SUCCESS;
   }
}

