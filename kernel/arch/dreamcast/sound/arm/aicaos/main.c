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

/* Initialize the OS */
void arm_main(void)
{
    /* Clear BSS section */
    memset(&__bss_start, 0,
	   (unsigned int)&__bss_end - (unsigned int)&__bss_start);

    main(0, NULL);
}
