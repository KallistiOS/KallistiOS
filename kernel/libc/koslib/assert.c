/* KallistiOS ##version##

   assert.c
   Copyright (C)2002 Megan Potter

*/

// In both the newlib and KOS libc cases, we use our own assert. The
// assert_msg and assert hooking functionality is just too useful.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <kos/dbglog.h>
#include <arch/stack.h>

/* The default assert handler */
static void __noreturn assert_handler_default(const char *file, int line,
                            const char *expr, const char *msg, const char *func) {
    dbglog(DBG_CRITICAL, "\n*** ASSERTION FAILURE ***\n");

    if(msg == NULL)
        dbglog(DBG_CRITICAL, "Assertion \"%s\" failed at %s:%d in `%s'\n\n",
               expr, file, line, func);
    else
        dbglog(DBG_CRITICAL, "Assertion \"%s\" failed at %s:%d in `%s': %s\n\n",
               expr, file, line, func, msg);

    if(__is_defined(FRAME_POINTERS))
        arch_stk_trace(2);

    abort();
    /* NOT REACHED */
}

/* The active assert handler */
static assert_handler_t a_hnd = assert_handler_default;

/* Set a new handler */
assert_handler_t assert_set_handler(assert_handler_t hnd) {
    assert_handler_t rv = a_hnd;
    a_hnd = hnd;
    return rv;
}

__used void __assert(const char *file, int line, const char *expr, const char *msg, const char *func) {
    if(a_hnd != NULL)
        a_hnd(file, line, expr, msg, func);
}

__used void __assert_func(const char *file, int line, const char *func, const char *expr) {
    __assert(file, line, expr, NULL, func);
}
