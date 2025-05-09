/*
   AICAOS

   main.c
   Copyright (C) 2025 Paul Cercueil

   AICAOS initialization code
*/

#include <aicaos/task.h>
#include <stddef.h>
#include <string.h>

extern int main(int argc, char **argv);

extern unsigned int __bss_start, __bss_end;
extern unsigned int __init_table_start, __init_table_end;

static struct task main_task;
static unsigned int main_task_stack[0x400];

/* Initialize the OS */
void arm_main(void)
{
    unsigned int *init_fn, args[4] = { 0 };

    /* Clear BSS section */
    memset(&__bss_start, 0,
           (unsigned int)&__bss_end - (unsigned int)&__bss_start);

    /* Run constructors */
    for (init_fn = &__init_table_start; init_fn != &__init_table_end; init_fn++)
        ((void (*)(void))*init_fn)();

    /* Register and add our main task */
    task_init(&main_task, "main", main, args, TASK_PRIO_LOW,
              main_task_stack, sizeof(main_task_stack));

    current_task = &main_task;
    __task_reschedule(0);
}
