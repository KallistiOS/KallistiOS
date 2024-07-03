/* KallistiOS ##version##

   reentrant_mutex.c

   Copyright (C) 2024 Eric Fradella
   Copyright (C) 2024 Falco Girgis

   Concurrency example that creates a "reentrant mutex" (aka recursive mutex)
   on top of KOS's basic mutex. 
   
   Normally, you would not want to do this, as KOS mutexes can simply be used
   with "MUTEX_TYPE_RECURSIVE" to achieve the same behavior with less work;
   however, this is actually the exact mechanism currently used by the Rust
   standard library to implement such functionality, so it's still a useful
   demonstration and serves as a validation test for the behavior of KOS's
   mutexes.

  The referenced Rust implementation is available at
  https://github.com/rust-lang/rust/blob/4bc39f02/library/std/src/sync/reentrant_lock.rs

 */

#include <kos.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdnoreturn.h>

/* Number of threads to spawn */
#define THREAD_COUNT     (DBL_MEM? 600 : 300)
 
/* % chance thread will pass when maybe_pass() called */
#define THREAD_PASS_CHANCE 75

/* Reentrant mutex we're implementing on top of KOS's normal mutex. */
typedef struct {
    mutex_t mutex;              /* Regular mutex */
    _Atomic kthread_t *owner;   /* Current mutex owner */
    unsigned count;             /* Reentrant count */
} reentrant_mutex_t;

/* Initializes our reentrant_mutex structure. */
static void reentrant_mutex_init(reentrant_mutex_t *rmutex) {
    mutex_init(&rmutex->mutex, MUTEX_TYPE_NORMAL);
    atomic_store(&rmutex->owner, NULL);
    rmutex->count = 0;
}

/* Uninitializes our reentrant_mutex structure. */
static void reentrant_mutex_uninit(reentrant_mutex_t *rmutex) {
    mutex_destroy(&rmutex->mutex);
}

/* Reports an error for the current thread and exits with failure value. */
noreturn static void failure(const char *fmt, ...) {
    char buffer[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    fprintf(stderr, "* * * FAILURE * * *\n");
    fprintf(stderr, "thread %s: %s\n", thd_get_label(thd_current), buffer);

    exit(EXIT_FAILURE);
}
 
/* Locks our reentrant_mutex structure. */
static void reentrant_mutex_lock(reentrant_mutex_t *rmutex) {
    /* If we are the owning thread, we must have already acquired the mutex. */
    if(atomic_load(&rmutex->owner) == (_Atomic kthread_t *)thd_current) {
        /* Increment the lock count. */
        rmutex->count++;

        /* Sanity check: mutex better be locked! */
        if(!mutex_is_locked(&rmutex->mutex))
            failure("Owns rmutex->mutex but it isn't locked!");
    } 
    else {
        kthread_t *expected = NULL;
        
        /* Attempt to acquire the mutex, since we are not the owning thread. */
        if(mutex_lock(&rmutex->mutex) < 0)
            failure("Failed to lock mutex: %s", strerror(errno));

        /* Set the owning thread to this thread. */
        atomic_compare_exchange_strong(&rmutex->owner, 
                                       &expected, 
                                       (_Atomic kthread_t *)thd_current);
        
        /* This should be impossible if our threading system is working properly. */
        if(rmutex->count)
            failure("rmutex->count was %u when it MUST be zero!", rmutex->count);

        rmutex->count++;
    }
}

/* Unlocks our reentrant_mutex structure. */
static void reentrant_mutex_unlock(reentrant_mutex_t *rmutex) {
    /* We better currently be the owning thread if we're attempting to unlock it! */
    if(atomic_load(&rmutex->owner) == (_Atomic kthread_t *)thd_current) {
        /* Decrement the lock counter. */
        if(atomic_fetch_sub(&rmutex->count, 1) == 1) {
            /* Clear the owning thread and release the mutex if counter hits 0. */
            atomic_store(&rmutex->owner, NULL);

            /* Unlock the mutex. */
            if(mutex_unlock(&rmutex->mutex) < 0)
                failure("Failed to unlock mutex: %s", strerror(errno));
        }

    } else {
        fprintf(stderr, "Error: Thread does not own the mutex\n");
    }
}

/* Use the provided probability to determine whether the current thread
   should pass/yield execution to another thread. */
static void maybe_pass(void) {
    unsigned value;

    getentropy(&value, sizeof(value));

    value %= 100;
    if(value < THREAD_PASS_CHANCE)
        thd_pass();
}

/* Mutex controlling access to "shared_variable." */
static reentrant_mutex_t rmutex;
/* Variable all threads are fighting over modifying. */
static int shared_variable = 0;
 
/* Exec function for each thread. Attempts to acquire the reentrant_mutex
   multiple times, potentially yielding in between each acquisition,
   and incrementing the shared variable once while the thread owns the lock. */
static void *thread_func(void *arg) {
    printf("Hello from thread %s!\n",
           thd_get_label(thd_current));

    maybe_pass();
    reentrant_mutex_lock(&rmutex);    /* lock count = 1 */

    maybe_pass();
    reentrant_mutex_lock(&rmutex);    /* lock count = 2 */

    shared_variable++;

    maybe_pass();
    reentrant_mutex_unlock(&rmutex);  /* lock count = 1 */

    maybe_pass();
    reentrant_mutex_lock(&rmutex);    /* lock count = 2 */

    maybe_pass();
    reentrant_mutex_unlock(&rmutex);  /* lock count = 1 */

    maybe_pass();
    reentrant_mutex_unlock(&rmutex);  /* unlocked */
 
    return NULL;
}

int main(int argc, const char* argv[]) {
    kthread_t *threads[THREAD_COUNT];
    
    /* Initialize our mutex */
    reentrant_mutex_init(&rmutex);
 
    /* Spawn a bunch of threads, potentially yielding the main thread after
       each one gets spawned. */
    for(size_t i = 0; i < THREAD_COUNT; ++i) {
        threads[i] = thd_create(false, thread_func, NULL);

        char thd_label[16];
        snprintf(thd_label, sizeof(thd_label), "%u", i);
        thd_set_label(threads[i], thd_label);
 
        maybe_pass();
    }
 
    /* Wait for each thread to complete. */
    for(size_t i = 0; i < THREAD_COUNT; ++i)
        thd_join(threads[i], NULL);

    /* Ensure our mutex is left in the expected state. */
    if(rmutex.count || rmutex.owner || mutex_is_locked(&rmutex.mutex))
        failure("Recursive mutex was left in unexpected state!");

    /* Clean up our mutex. */
    reentrant_mutex_uninit(&rmutex);
 
    printf("Shared variable is %d!\n", shared_variable);
 
    if(shared_variable == THREAD_COUNT)
        printf("Reentrant lock test completed successfully!\n");
    else
        failure("Shared variable != THREAD_COUNT!");

    return EXIT_SUCCESS;
}
