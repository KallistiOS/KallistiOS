/* KallistiOS ##version##

   stacktrace.c
   (c)2002 Megan Potter
*/

#include <kos.h>
#include <assert.h>

/*

This example shows off how to setup an assert handler (for example for
a custom crash screen, or some other debug method, or even ignoring
asserts selectively). It also demonstrates the default assert handler.
If you compiled with -DFRAME_POINTERS and without -fomit-frame-pointer
then you'll get something like this:

*** ASSERTION FAILURE ***
Assertion "a != 5" failed at asserthnd.c:40 in `func2': This is a test message!

-------- Stack Trace (innermost first) ---------
   8c01016a
   8c0101e0
   8c010478
   8c010062
-------------- End Stack Trace -----------------

You can punch those numbers into addr2line to get a nice stack traceback,
assuming you compiled your program with -g:

>] dc-addr2line -e asserthnd.elf 8c01016a 8c0101e0 8c010478
/usr/local/home/bard/prj/kos/examples/dreamcast/basic/asserthnd/asserthnd.c:46
/usr/local/home/bard/prj/kos/examples/dreamcast/basic/asserthnd/asserthnd.c:69
/usr/local/home/bard/prj/kos/kernel/arch/dreamcast/kernel/main.c:129

*/

/*  These are marked as __noinline to ensure the compiler
    doesn't try to get smart and inline them which would
    defeat the purpose of the example.
*/
__noinline void func2(void) {
    int a = 5;

    assert_msg(a != 5, "This is a test message!");
    assert(a != 5);
}

__noinline void func1(void) {
    func2();
}

void hnd(const char * file, int line, const char * expr,
         const char * msg, const char * func) {
    printf("Our assert handler got called!\n"
           "  file = %s\n"
           "  line = %d\n"
           "  expr = %s\n"
           "  msg = %s\n"
           "  func = %s\n",
           file, line, expr, msg, func);
}

int main(int argc, char **argv) {
    /* Try once with our own handler */
    assert_handler_t old = assert_set_handler(hnd);
    func1();

    /* Now put back the default */
    assert_set_handler(old);
    func1();

    return 0;
}


