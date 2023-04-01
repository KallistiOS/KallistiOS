/* KallistiOS ##version##

   atexit.c
   Copyright (C)2002 Florian Schulze
   Copyright (C)2003 Megan Potter
*/

#include <stdlib.h>


struct arch_atexit_handler {
    struct arch_atexit_handler *next;
    void (*handler)(void);
};

struct arch_atexit_handler *arch_atexit_handlers = NULL;

int atexit(void (*func)(void)) {
    struct arch_atexit_handler *new_handler;

    if(!func)
        return -1;

    new_handler = malloc(sizeof(struct arch_atexit_handler));

    if(!new_handler)
        return -1; /* TODO: error handling? */

    new_handler->handler = func;
    new_handler->next = arch_atexit_handlers;

    arch_atexit_handlers = new_handler;

    return 0;
}

/* Call all the atexit() handlers */
void _atexit_call_all() {
    struct arch_atexit_handler *exit_handler;

    exit_handler = arch_atexit_handlers;

    while(exit_handler) {
        if(exit_handler->handler)
            exit_handler->handler();

        exit_handler = exit_handler->next;
        free(arch_atexit_handlers);
        arch_atexit_handlers = exit_handler;
    }
}


