#ifndef TEST_THREAD_H
#define TEST_THREAD_H
#include <pthread.h>
#include <stdbool.h>
typedef struct test_thread {
    pthread_t id;
    void *(*routine)(void *);
    void *data;
} kthread_t;
typedef struct { const char *label; } kthread_attr_t;
kthread_t *thd_create_ex(const kthread_attr_t *, void *(*)(void *), void *);
kthread_t *thd_get_current(void);
int thd_join(kthread_t *, void **);
extern _Thread_local bool test_irq;
static inline bool irq_inside_int(void) { return test_irq; }
#endif
