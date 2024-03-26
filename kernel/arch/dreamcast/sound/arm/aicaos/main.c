/*
   AICAOS

   main.c
   Copyright (C) 2025 Paul Cercueil

   AICAOS initialization code
*/

#include <stddef.h>

extern int main(int argc, char **argv);

/* Initialize the OS */
void arm_main(void)
{
    main(0, NULL);
}
