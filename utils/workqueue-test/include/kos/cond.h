#ifndef TEST_COND_H
#define TEST_COND_H
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdatomic.h>
#include <time.h>
#include <kos/mutex.h>
typedef pthread_cond_t condvar_t;
#define COND_INITIALIZER PTHREAD_COND_INITIALIZER
extern atomic_int test_long_waits;
static inline int cond_destroy(condvar_t *c) { return pthread_cond_destroy(c); }
static inline int cond_signal(condvar_t *c) { return pthread_cond_signal(c); }
static inline int cond_broadcast(condvar_t *c) { return pthread_cond_broadcast(c); }
static inline int cond_wait(condvar_t *c, mutex_t *m) { return pthread_cond_wait(c, m); }
static inline int cond_wait_timed(condvar_t *c, mutex_t *m, int ms) {
    struct timespec deadline;
    assert(ms >= 0);
    if(!ms) return cond_wait(c, m);
    if(ms == INT_MAX) atomic_fetch_add(&test_long_waits, 1);
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += ms / 1000;
    deadline.tv_nsec += (long)(ms % 1000) * 1000000L;
    if(deadline.tv_nsec >= 1000000000L) {
        ++deadline.tv_sec;
        deadline.tv_nsec -= 1000000000L;
    }
    int rv = pthread_cond_timedwait(c, m, &deadline);
    if(rv) { errno = rv; return -1; }
    return 0;
}
#endif
