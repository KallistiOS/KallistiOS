#ifndef _TASK_H
#define _TASK_H

#define __noreturn __attribute__((noreturn))

#define DEFAULT_STACK_SIZE 4096

struct context {
    /* XXX: don't change the order */
    unsigned int r0_r7[8];
    unsigned int pc;
    unsigned int r8_r14[7];
    unsigned int cpsr;
};

enum task_state {
    TASK_DEAD,
    TASK_RUNNING,
};

enum task_prio {
    TASK_PRIO_HIGHEST,
    TASK_PRIO_HIGH,
    TASK_PRIO_NORMAL,
    TASK_PRIO_LOW,
    TASK_PRIO_LOWEST,
    TASK_PRIO_IDLE,
    TASK_PRIO_COUNT,
};

struct task {
    struct context context;
    struct task *next;
    unsigned int id;
    enum task_state state;
    const char *name;
};

/* Pointer to the current task */
extern struct task *current_task;

/* Initialize and start a new task */
void task_init(struct task *task, const char *name, void *func,
               unsigned int params[4], enum task_prio prio,
               unsigned int *stack, unsigned int stack_size);

/* Request a reschedule. */
void task_reschedule(void);

/* Yield the task.
 * The difference with a reschedule, is that yielding will cause the scheduler
 * to always pick a different task. */
void task_yield(void);

/* Reschedule without saving the current task. */
__noreturn void __task_reschedule(_Bool skip_me);

#endif
