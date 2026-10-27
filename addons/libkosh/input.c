/*
 * input.c
 *
 * (C) 2000 Jordan DeLong
 */

#include <string.h>
#include "kosh.h"
#include "builtin.h"

/* read command line, try to execute builtin commands, otherwise externals (this coming soon) */
void input_oneloop(void) {
    int argc;
    char *argv[20];     /* limit to 20 args (should be plenty) */
    char *str;
    char buff[256];

    /* Wait for a line of input */
    conio_printf("$ ");
    if (conio_input_getline(-1, buff, 255) < 0) {
        kosh_exit = 1;
        return;
    }

    str = buff;

    /* we don't care if the user just hit enter */
    if (buff[0] == '\0')
        return;

    /* seperate the string into args */
    for (argc = 0; argc < 20;) {
        if ((argv[argc] = strsep(&str, " \t\n")) == NULL)
            break;
        if (*argv[argc] != '\0')
            argc++;
    }

    /* try to run the command as a builtin */
    if (!kosh_builtin_command(argc, argv)) {
        /* makeabspath(buff, argv[0], NAME_MAX); */

        /* the builtin failed so we try an external */
        /* if (process->load_and_exec(buff, argc, argv))
            conio_printf("kosh: %s: command not found\n", buff); */
        conio_printf("Sorry, OS mode not available yet\n");
    }
}

