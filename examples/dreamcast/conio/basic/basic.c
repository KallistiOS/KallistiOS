/* KallistiOS ##version##

   basic.c

   (c)2002 Megan Potter

   A simple conio example

 */

#include <kos.h>
#include <conio/conio.h>

/* The main program */
int main(int argc, char **argv) {
    char buffer[256];

    /* If the user hits start, bail */
    cont_btn_callback(0, CONT_START, (cont_btn_callback_t)arch_exit);

    /* Setup the console */
    pvr_init_defaults();
    conio_init(CONIO_TTY_PVR, CONIO_INPUT_LINE);

    conio_printf("Basic Conio test.\n"
                 "Press START on any controller or type 'exit' to quit.\n");

    for(; ;) {
        conio_printf("> ");
        conio_input_getline(-1, buffer, 256);
        conio_printf("User entered: '%s'\n", buffer);

        if(!strcmp(buffer, "exit"))
            break;
    }

    return 0;
}
