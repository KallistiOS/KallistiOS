/*
   AICAOS

   main.c
   Copyright (C) 2025 Paul Cercueil

   AICAOS initialization code
*/

#include <stddef.h>
#include <string.h>

extern int main(int argc, char **argv);

extern unsigned int __bss_start, __bss_end;
extern unsigned int __init_table_start, __init_table_end;

/* Initialize the OS */
void arm_main(void)
{
    unsigned int *init_fn;

    /* Clear BSS section */
    memset(&__bss_start, 0,
	   (unsigned int)&__bss_end - (unsigned int)&__bss_start);

    /* Run constructors */
    for (init_fn = &__init_table_start; init_fn != &__init_table_end; init_fn++)
        ((void (*)(void))*init_fn)();

    main(0, NULL);
}
