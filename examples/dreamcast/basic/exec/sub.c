/* KallistiOS ##version##

   sub.c
   (c)2002 Megan Potter
*/

#include <kos.h>

/* Your program's main entry point */
int main(int argc, char **argv) {
    printf("\n\nHello world from sub.bin:\n");
    for(int a = 0; a < argc; ++a) {
        printf("argv[%d]: %s\n", a, argv[a]);
    }
    fflush(stdout);
    return 0;
}


