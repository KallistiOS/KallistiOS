#ifndef TEST_KOS_THREAD_H
#define TEST_KOS_THREAD_H
#include <stdint.h>
#include <sys/types.h>
#define KOS_PID 1
typedef struct kthread { int token; } kthread_t;
kthread_t *thd_get_current(void);
uint64_t thd_get_cpu_time(kthread_t *thread);
uint64_t thd_get_total_cpu_time(void);
#endif
