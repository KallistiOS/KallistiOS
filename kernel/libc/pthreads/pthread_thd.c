#include <pthread.h>
#include <errno.h>
#include <assert.h>

#include <kos/thread.h>

/* Thread Creation, P1003.1c/Draft 10, p. 144 */

int pthread_create(pthread_t *thread, const pthread_attr_t  *attr,
                   void * (*start_routine)(void *), void *arg) {
    kthread_t * nt;

    (void)attr;

    assert(thread);
    assert(start_routine);

    nt = thd_create(0, start_routine, arg);

    if(nt) {
        *thread = nt;
        return 0;
    }
    else {
        return EAGAIN;
    }
}

/* Wait for Thread Termination, P1003.1c/Draft 10, p. 147 */

int pthread_join(pthread_t thread, void **value_ptr) {
    assert(thread);

    if(thd_join(thread, value_ptr) < 0)
        return ESRCH;
    else
        return 0;
}

/* Detaching a Thread, P1003.1c/Draft 10, p. 149 */

int pthread_detach(pthread_t thread) {
    int rv = thd_detach(thread);

    if(rv == -3) {
        return EINVAL;
    }
    else if(rv < 0) {
        return ESRCH;
    }
    else {
        return 0;
    }
}

/* Thread Termination, p1003.1c/Draft 10, p. 150 */

void pthread_exit(void *value_ptr) {
    thd_exit(value_ptr);
}

/* Get Calling Thread's ID, p1003.1c/Draft 10, p. XXX */

pthread_t pthread_self(void) {
    return thd_current;
}

/* Compare Thread IDs, p1003.1c/Draft 10, p. 153 */

int pthread_equal(pthread_t t1, pthread_t t2) {
    return t1 == t2;
}

int pthread_atfork(void (*prepare)(void), void (*parent)(void),
                   void (*child)(void)) {
    (void)prepare; (void)parent; (void)child;
    return 0;
}
