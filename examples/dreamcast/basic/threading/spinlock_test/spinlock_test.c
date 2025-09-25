/* KallistiOS ##version##

   spinlock_test.c

   Copyright (C) 2023 Falco Girgis

    This file is a simple example of the different 
    ways in which to use a spinlock. It serves two 
    purposes:
    
        1) To demonstrate their usage
        2) To serve as a test for KOS's implementation

    Multiple threads are spawned and then take turns
    waiting for the spinlock to become available, using
    two different methods of waiting.
 */

#include <kos.h>
#include <stdlib.h>
#include <kos/spinlock.h>

/* 
    Simple macro that calls printf and immediately flushes its 
    buffer so we don't have to wait for it to fill before seeing
    a thread's output. 
*/
#define print(...) \
    do { \
        printf(__VA_ARGS__); \
        fflush(stdout); \
    } while(0)


/* The counter out threads will be competing for access to */
static uint32_t lock_counter = 0;
/* The spinlock our threads will use to manage access to the counter */
static spinlock_t lock = SPINLOCK_INITIALIZER;

/* 
   Calculates the Nth Fibonacci number. 
   Lets give the threads something to do while they wait!
*/
static int fib(int n) {
    if (n <= 1)
        return n;

    return fib(n - 1) + fib(n - 2);
}

/* 
    Main execution path for each thread, demonstrating the following
    with a spinlock:

        1) Doing something useful while waiting for the lock
        2) Simply locking normally and waiting for it
*/
void *thd(void *v) {
    unsigned tid = (unsigned)v;
    unsigned fibres = 0, fibn = 0;

    /* Continue calling spinlock_trylock() until eventually 
       locking succeeds. */
    while(!spinlock_trylock(&lock)) {
        print("Thread[%u] still trying the lock!\n", tid);
        /* We can do something else while we wait! */
        fibres = fib(fibn++);
        /* Let the other threads have some time too. */
        thd_pass();
    }

    print("Thread[%u] trylock succeeded!\n", tid);
    print("Thread[%u] calculated the %uth fibonacci number "
          "while waiting: %u\n", tid, fibn - 1, fibres);

    /* Increment our counter once now that we have the lock. */
    ++lock_counter;
    sleep(1);

    print("Thread[%u] yielding the lock\n", tid);

    /* Release the lock so the other threads can have it. */
    spinlock_unlock(&lock);

    sleep(1);

    /* Attempt to gain the lock again, this time by
       locking it. Program execution will not proceed
       until the lock is in the thread's possession. */
    print("Thread[%u] locking the lock\n", tid);
    spinlock_lock(&lock);
    print("Thread[%u] locked the lock\n", tid);

    /* Increment out counter again to show we got the lock
       a second time. */
    ++lock_counter; 
    sleep(1);

    print("Thread[%u] unlocking the lock\n", tid);
    spinlock_unlock(&lock);

    /* Return how far we got into the Fibonacci sequence
       while we waited for the lock. */
    return (void *)(fibn - 1);
}

int main(int argc, char **argv) {
    const int thread_count = 10;
    kthread_t *threads[thread_count]; 
    int i, join_error = 0;
    unsigned fibcount = 0;

    print("Starting Threads\n");

    /* Kick off a number of threads to all compete for our spinlock. */
    for (i = 0; i < thread_count; i++) {
        threads[i] = thd_create(0, thd, (void *)(i + 1));
    }

    /* Perform the same logic for the main thread. */
    thd((void*)0);

     /* Wait for each thread to return. */
    for(i = 0; i < thread_count; i++) {
        void* result;

        if(thd_join(threads[i], &result) != 0) {
            /* Mark the test as failed if the thread failed to join gracefully. */
            fprintf(stderr, "Thread[%i] failed to complete properly!\n", i + 1);
            join_error = 1;
        } else {
            print("Thread[%d] returned.\n", i + 1);
            /* Add the number of Fibonacci numbers it calculated to our total. */
            fibcount += (unsigned)result;
        }
    }
    
    print("Threads finished and calculated %u fibonacci numbers while they waited!\n", fibcount);

    /* Ensure there were no issues with threads exiting gracefully,
       and that the lock_counter was incremented twice for every 
       thread, plus 2 more for the main thread. 
    */
    if(join_error || lock_counter != (thread_count + 1) * 2) {
        fprintf(stderr, "\n\n***** SPINLOCK TEST FAILED! *****\n\n");
        return EXIT_FAILURE;
    }
    else {
        print("\n\n***** SPINLOCK TEST SUCCESS! *****\n\n");
        return EXIT_SUCCESS;
    }
}
