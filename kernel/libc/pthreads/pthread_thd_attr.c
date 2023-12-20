#include <pthread.h>
#include <errno.h>

/* Thread Creation Scheduling Attributes, P1003.1c/Draft 10, p. 120 */

int pthread_attr_setscope(pthread_attr_t *attr, int contentionscope) {
    (void)attr;
    (void)contentionscope;
    return 0;
}

int pthread_attr_getscope(const pthread_attr_t *attr, int *contentionscope) {
    (void)attr;
    (void)contentionscope;
    return EINVAL;
}

int pthread_attr_setinheritsched(pthread_attr_t *attr, int inheritsched) {
    (void)attr;
    (void)inheritsched;
    return 0;
}

int pthread_attr_getinheritsched(const pthread_attr_t *attr, int *inheritsched) {
    (void)attr;
    (void)inheritsched;
    return EINVAL;
}

int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy) {
    (void)attr;
    (void)policy;
    return 0;
}

int pthread_attr_getschedpolicy(const pthread_attr_t *attr, int *policy) {
    (void)attr;
    (void)policy;
    return EINVAL;
}


int pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param) {
    (void)attr;
    (void)param;
    return 0;
}

int pthread_attr_getschedparam(const pthread_attr_t *attr, struct sched_param *param) {
    (void)attr;
    (void)param;
    return EINVAL;
}

/* Dynamic Thread Scheduling Parameters Access, P1003.1c/Draft 10, p. 124 */

int pthread_getschedparam(pthread_t thread, int *policy, struct sched_param *param) {
    (void)thread;
    (void)policy;
    (void)param;
    return EINVAL;
}

int pthread_setschedparam(pthread_t thread, int policy, struct sched_param *param) {
    (void)thread;
    (void)policy;
    (void)param;
    return 0;
}


/* Thread Creation Attributes, P1003.1c/Draft 10, p, 140 */

int pthread_attr_init(pthread_attr_t *attr) {
    (void)attr;
    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr) {
    (void)attr;
    return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize) {
    (void)attr;
    (void)stacksize;
    return EINVAL;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize) {
    (void)attr;
    (void)stacksize;
    return 0;
}

int pthread_attr_getstackaddr(const pthread_attr_t *attr, void **stackaddr) {
    (void)attr;
    (void)stackaddr;
    return EINVAL;
}

int pthread_attr_setstackaddr(pthread_attr_t  *attr, void *stackaddr) {
    (void)attr;
    (void)stackaddr;
    return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate) {
    (void)attr;
    (void)detachstate;
    return EINVAL;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {
    (void)attr;
    (void)detachstate;
    return 0;
}

/* Execution of a Thread, P1003.1c/Draft 10, p. 181 */
int pthread_cancel(pthread_t thread) {
    (void)thread;
    return EINVAL;
}

/* Setting Cancelability State, P1003.1c/Draft 10, p. 183 */

int pthread_setcancelstate(int state, int *oldstate) {
    (void)state;
    (void)oldstate;
    return EINVAL;
}

int pthread_setcanceltype(int type, int *oldtype) {
    (void)type;
    (void)oldtype;
    return EINVAL;
}

void pthread_testcancel(void) {
}

/* Establishing Cancellation Handlers, P1003.1c/Draft 10, p. 184 */

void pthread_cleanup_push(void (*routine)(void *), void *arg) {
    (void)routine;
    (void)arg;
}

void pthread_cleanup_pop(int execute) {
    (void)execute;
}

/* Accessing a Thread CPU-time Clock, P1003.4b/D8, p. 58 */

// This isn't even man'd in Linux...
/* int pthread_getcpuclockid(pthread_t thread_id, clockid_t *clock_id) {
} */

/* CPU-time Clock Thread Creation Attribute, P1003.4b/D8, p. 59 */

int pthread_attr_setcputime(pthread_attr_t *attr, int clock_allowed) {
    (void)attr;
    (void)clock_allowed;
    return 0;
}

int pthread_attr_getcputime(pthread_attr_t *attr, int *clock_allowed) {
    (void)attr;
    (void)clock_allowed;
    return EINVAL;
}
