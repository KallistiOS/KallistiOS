#ifndef __KOS_MUTEX_H
#define __KOS_MUTEX_H

#include <pthread.h>

#define MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

typedef pthread_mutex_t mutex_t;

static inline int mutex_lock(mutex_t *mutex) {
    return pthread_mutex_lock(mutex);
}

static inline int mutex_lock_irqsafe(mutex_t *mutex) {
    return pthread_mutex_lock(mutex);
}

static inline int mutex_unlock(mutex_t *mutex) {
    return pthread_mutex_unlock(mutex);
}

#endif
