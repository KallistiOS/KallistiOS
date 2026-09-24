#ifndef TEST_MUTEX_H
#define TEST_MUTEX_H
#include <assert.h>
#include <pthread.h>
typedef pthread_mutex_t mutex_t;
#define MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
static inline int mutex_lock(mutex_t *m) { return pthread_mutex_lock(m); }
static inline int mutex_unlock(mutex_t *m) { return pthread_mutex_unlock(m); }
static inline int mutex_destroy(mutex_t *m) { return pthread_mutex_destroy(m); }
static inline mutex_t *test_lock(mutex_t *m) { assert(!mutex_lock(m)); return m; }
static inline void test_unlock(mutex_t **m) { assert(!mutex_unlock(*m)); }
#define mutex_lock_scoped(m) \
    mutex_t *test_scoped_lock __attribute__((cleanup(test_unlock))) = test_lock(m)
#endif
