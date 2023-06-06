// This can be useful for checking whether some pthread program compiled
// correctly (e.g. libstdc++).
// #define MUTEX_DEBUG 1

#include <pthread.h>
#include <errno.h>
#include <assert.h>

#include <kos/mutex.h>

// XXX Recursive mutexes are not supported ... this could cause deadlocks
// in code expecting it. Where do you set that!?

/* Mutex Initialization Attributes, P1003.1c/Draft 10, p. 81 */

int pthread_mutexattr_init(pthread_mutexattr_t *attr) {
    (void)attr;
    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr) {
    (void)attr;
    return 0;
}

int pthread_mutexattr_getpshared(const pthread_mutexattr_t *attr, int  *pshared) {
    (void)attr;
    (void)pshared;
    return 0;
}

int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared) {
    (void)attr;
    (void)pshared;
    return 0;
}

/* Initializing and Destroying a Mutex, P1003.1c/Draft 10, p. 87 */

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
    (void)attr;
    assert(mutex);

    return mutex_init(mutex, MUTEX_TYPE_NORMAL);
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    assert(mutex);

    return mutex_destroy(mutex);
}

/*  Locking and Unlocking a Mutex, P1003.1c/Draft 10, p. 93
    NOTE: P1003.4b/D8 adds pthread_mutex_timedlock(), p. 29 */

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    assert(mutex);

    return mutex_lock(mutex);
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    assert(mutex);

    return mutex_trylock(mutex);
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    assert(mutex);

    return mutex_unlock(mutex);
}

/* Mutex Initialization Scheduling Attributes, P1003.1c/Draft 10, p. 128 */

int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol) {
    (void)attr;
    (void)protocol;
    return 0;
}

int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *attr, int *protocol) {
    (void)attr;
    (void)protocol;
    return EINVAL;
}

int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *attr, int prioceiling) {
    (void)attr;
    (void)prioceiling;
    return 0;
}

int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *attr, int *prioceiling) {
    (void)attr;
    (void)prioceiling;
    return EINVAL;
}

/* Change the Priority Ceiling of a Mutex, P1003.1c/Draft 10, p. 131 */

int pthread_mutex_setprioceiling(pthread_mutex_t *mutex, int prioceiling, int *old_ceiling) {
    (void)mutex;
    (void)prioceiling;
    (void)old_ceiling;
    return 0;
}

int pthread_mutex_getprioceiling(pthread_mutex_t *mutex, int *prioceiling) {
    (void)mutex;
    (void)prioceiling;
    return EINVAL;
}
