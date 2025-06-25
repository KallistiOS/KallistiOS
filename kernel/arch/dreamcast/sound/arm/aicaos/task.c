/*
   AICAOS

   task.c
   Copyright (C) 2025 Paul Cercueil

   AICAOS threading system
*/

#include <aicaos/aica.h>
#include <aicaos/init.h>
#include <aicaos/irq.h>
#include <aicaos/task.h>
#include <stddef.h>

static struct task idle_task;
struct task *current_task;

static unsigned short last_pos;
static unsigned int task_counter = 0;
static struct task * tasks[TASK_PRIO_COUNT];

static struct task *wait_queue;

/* Inside task_asm.S */
__noreturn void task_select(struct context *context);

static void idle_function(void)
{
    for (;;)
        task_reschedule();
}

static void aica_init_tasks(void)
{
    task_init(&idle_task, "idle", idle_function, NULL, TASK_PRIO_IDLE, NULL, 0);
}
aicaos_initcall(aica_init_tasks);

static void task_wakeup(unsigned short ticks)
{
    struct task *task;
    unsigned int i;

    for (i = 0; i < TASK_PRIO_COUNT; i++) {
        for (task = tasks[i]; task; task = task->next) {
            if (task->state == TASK_SLEEPING || task->state == TASK_WAIT_UNTIL) {
                if (task->wakeup > ticks)
                    task->wakeup -= ticks;
                else
                    task->state = TASK_RUNNING;
            }
        }
    }
}

static void task_program_next_wakeup(void)
{
    unsigned int i, div = 0, wakeup = DEFAULT_TIMEOUT_WAKEUP;
    struct task *task;

    for (i = 0; i < TASK_PRIO_COUNT; i++) {
        for (task = tasks[i]; task; task = task->next)
            if (task->state == TASK_RUNNING)
                break;

        if (task)
            break;

        for (task = tasks[i]; task; task = task->next) {
            if (task->state == TASK_SLEEPING && task->wakeup < wakeup)
                wakeup = task->wakeup;
        }
    }

    while (wakeup > 255) {
        wakeup >>= 1;
        div++;
    }

    /* Re-program the timer to the next event */
    SPU_REG32(REG_SPU_TIMER0_CTRL) =
        SPU_FIELD_PREP(SPU_TIMER_CTRL_START, 256 - wakeup) |
        SPU_FIELD_PREP(SPU_TIMER_CTRL_DIV, div);

    /* Re-enable timer */
    SPU_REG32(REG_SPU_INT_RESET) = SPU_INT_ENABLE_TIMER0;
}

static __noreturn void __task_select(struct task *task)
{
    irq_disable();

    current_task = task;
    task_select(&task->context);
}

/* Called from task_asm.S */
__noreturn void __task_reschedule(bool skip_me)
{
    unsigned short counter, ticks;
    struct task *task;
    unsigned int i;

    irq_disable();

    /* Cancel previous wakeup timer */
    SPU_REG32(REG_SPU_TIMER0_CTRL) = 0;

    counter = aica_read_counter();
    ticks = counter - last_pos;
    last_pos = counter;

    current_task->cpu_time += ticks;

    /* Wake up sleeping tasks, and program next wakeup */
    task_wakeup(ticks);
    task_program_next_wakeup();

    for (i = 0; i < TASK_PRIO_COUNT; i++) {
        for (task = tasks[i]; task; task = task->next) {
            if (skip_me && task == current_task)
                continue;

            if (task->state == TASK_RUNNING)
                __task_select(task);
        }
    }

    __builtin_unreachable();
}

/* Called from task_asm.S */
__noreturn void __task_exit(void)
{
    struct task *task, *prev = NULL;
    unsigned int i;

    irq_disable();

    for (i = 0; i < TASK_PRIO_COUNT; i++) {
        for (task = tasks[i]; task; prev = task, task = task->next) {
            if (task == current_task) {
                /* Remove the task from the runnables list */
                if (prev)
                    prev->next = task->next;
                else
                    tasks[i] = task->next;

                task->state = TASK_DEAD;
                task_wake(task, 1);
                break;
            }
        }

        if (task)
            break;
    }

    __task_reschedule(0);
}

void task_init(struct task *task, const char *name, void *func,
               unsigned int params[4], enum task_prio prio,
               unsigned int *stack, unsigned int stack_size)
{
    unsigned int i;

    for (i = 0; params && i < 4; i++)
        task->context.r0_r7[i] = params[i];

    /* Set the stack pointer */
    task->context.r8_r14[5] = (unsigned int)stack + stack_size;
    task->context.r8_r14[6] = (unsigned int)task_exit;
    task->context.pc = (unsigned int)func + 4;
    task->context.cpsr = 0x13; /* supervisor */
    task->state = TASK_RUNNING;
    task->name = name;
    task->wait_next = NULL;
    task->awaken = 0;
    task->prio = prio;
    task->real_prio = prio;
    task->cpu_time = 0;

    irq_disable_scoped();

    task->id = task_counter++;

    task->next = tasks[prio];
    tasks[prio] = task;
}

void task_sleep(ticks_t ticks)
{
    if (ticks) {
        current_task->wakeup = ticks;
        current_task->state = TASK_SLEEPING;

        task_reschedule();
    }
}

bool task_wait_timeout(void *obj, ticks_t ticks)
{
    irq_disable_scoped();

    current_task->wakeup = ticks;
    if (ticks)
        current_task->state = TASK_WAIT_UNTIL;
    else
        current_task->state = TASK_WAIT;

    current_task->awaken = 0;
    current_task->wait_obj = obj;

    /* Add current task to the wait queue */
    current_task->wait_next = wait_queue;
    wait_queue = current_task;

    task_reschedule();

    return current_task->awaken;
}

void task_wake(void *obj, bool all)
{
    struct task *task, *prev = NULL;

    irq_disable_scoped();

    for (task = wait_queue; task; prev = task, task = task->wait_next) {
        if ((task->state == TASK_WAIT || task->state == TASK_WAIT_UNTIL)
            && task->wait_obj == obj) {
            /* Remove task from wait queue */
            if (prev)
                prev->next = task->wait_next;
            else
                wait_queue = task->wait_next;

            /* We're running again */
            task->state = TASK_RUNNING;
            task->awaken = 1;

            if (!all)
                break;
        }
    }
}

void task_join(struct task *task)
{
    irq_disable_scoped();

    while (task->state != TASK_DEAD)
        task_wait(task);
}

static void task_set_prio(struct task *task, enum task_prio prio)
{
    struct task *tmp, *prev = NULL;
    enum task_prio curr = task->prio;

    for (tmp = tasks[curr]; tmp; prev = tmp, tmp = tmp->next) {
        if (tmp == task) {
            if (prev)
                prev->next = tmp->next;
            else
                tasks[curr] = tmp->next;
            break;
        }
    }

    task->prio = prio;
    task->next = tasks[prio];
    tasks[prio] = task;
}

void task_boost(struct task *task)
{
    irq_disable_scoped();

    if (current_task->prio < task->prio)
        task_set_prio(task, current_task->prio);
}

void task_unboost(void)
{
    irq_disable_scoped();

    if (current_task->prio != current_task->real_prio)
        task_set_prio(current_task, current_task->real_prio);
}

ticks_t task_get_cputime(void)
{
    uint16_t counter, ticks;

    irq_disable_scoped();

    counter = aica_read_counter();
    ticks = counter - last_pos;

    return current_task->cpu_time + ticks;
}
