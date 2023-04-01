/* KallistiOS ##version##

   fpu_exc.c
   (c)2002 Megan Potter
*/

#include <kos.h>

asm(
    "	.text\n"
    "	.align 2\n"
    "__get_fpscr:\n"
    "	rts\n"
    "	sts	fpscr,r0\n"
);

extern uint32 _get_fpscr();

int main(int argc, char **argv) {
    uint32 fpscr;
    double d = 1.0;

    /* Exit parachute */
    cont_btn_callback(0, CONT_START, (cont_btn_callback_t)arch_exit);

    fpscr = _get_fpscr();
    printf("fpscr is %08x\n", fpscr);

    while(d > 0.0) {
        printf("%f\n", d);
        d *= 0.01;
    }

    printf("Underflow occurred, no exceptions!\n");

    return 0;
}


