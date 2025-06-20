/* KallistiOS ##version##

   stacktrace.c
   (c)2002 Megan Potter
*/

#include <kos.h>

/*  This is marked as __noinline to ensure the compiler
    doesn't try to get smart and inline it which would
    defeat the purpose of the example.
*/
__noinline void func(void) {
    arch_stk_trace(0);
}

int main(int argc, char **argv) {
    arch_stk_trace(0);

    func();

    return 0;
}


