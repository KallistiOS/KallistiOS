/* KallistiOS ##version##

   assert.h
   Copyright (C)2002,2004 Megan Potter

*/

#ifndef __ASSERT_H
#define __ASSERT_H

#include <kos/cdefs.h>
__BEGIN_DECLS

/**
    \file   assert.h
    \brief  Standard C Assertions

    This file contains the standard C assertions to raise an assertion or to
    change the assertion handler.

    \author Megan Potter
*/

/* This is nice and simple, modeled after the BSD one like most of KOS;
   the addition here is assert_msg(), which allows you to provide an
   error message. */
#define _assert(e) assert(e)

#ifdef NDEBUG
#   define assert(e) ((void)0)
#   define assert_msg(e, m) ((void)0)
#else

/* This bit of magic borrowed from Newlib's assert.h... */
/* \cond */
#ifndef __ASSERT_FUNC
#if defined(__cplusplus)
#   define __ASSERT_FUNC __PRETTY_FUNCTION__
#elif __STDC_VERSION__ >= 199901L
#   define __ASSERT_FUNC __func__
#elif __GNUC__ >= 2
#   define __ASSERT_FUNC __FUNCTION__
#else
#   define __ASSERT_FUNC ((char *)0)
#endif
#endif
/* \endcond */

/** \brief  Standard C assertion macro.

    This macro does a standard C assertion, wherein the expression is evaluated,
    and if false, the program is ultimately aborted using abort(). If the
    expression evaluates to true, the macro does nothing (other than any side
    effects of evaluating the expression).

    \param  e               A value or expression to be evaluated as true or
                            false.
*/
#   define assert(e)        ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e, NULL, __ASSERT_FUNC))

/** \brief  assert() with a custom message.

    This macro acts the same as the assert() macro, but allows you to specify a
    custom message to be printed out if the assertion fails.

    \param  e               A value or expression to be evaluated as true or
                            false.
    \param  m               A message (const char *).
*/
#   define assert_msg(e, m) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e, m, __ASSERT_FUNC))
#endif

/* \cond */
/* Defined in assert.c */
void __assert(const char *file, int line, const char *expr,
              const char *msg, const char *func);
/* \endcond */

/** \brief  Assertion handler type.

    The user can provide their own assertion handler with this type. If none is
    provided, a default is used which ultimately prints out the location of the
    failed assertion and calls abort().

    \param  file            The filename where the assertion happened.
    \param  line            The line number where the assertion happened.
    \param  expr            The expression that raised the assertion.
    \param  msg             A custom message for why the assertion happened.
    \param  func            The function name from which the assertion happened.

    \see    assert_set_handler
*/
typedef void (*assert_handler_t)(const char * file, int line, const char * expr,
                                 const char * msg, const char * func);

/** \brief  Set an assertion handler to call on a failed assertion.

    The default assertion handler simply will print a message and call abort().

    \return                 The old assertion handler so it may be restored
                            later if appropriate.

    \see    assert_handler_t
*/
assert_handler_t assert_set_handler(assert_handler_t hnd);

__END_DECLS

#endif  /* __ASSERT_H */
