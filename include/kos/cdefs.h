/* KallistiOS ##version##

   kos/cdefs.h
   Copyright (C) 2002, 2004 Megan Potter
   Copyright (C) 2020, 2023 Lawrence Sebald
   Copyright (C) 2023 Falco Girgis

   Based loosely around some stuff in BSD's sys/cdefs.h
*/

/** \file    kos/cdefs.h
    \brief   Various common macros used throughout the codebase
    \ingroup system

    This file contains various convenience macros. Mostly compiler
    __attribute__ directives, as well as other language defines, and
    useful language extensions.

    \author Megan Potter
    \author Lawrence Sebald
    \author Falco Girgis
*/

#ifndef __KOS_CDEFS_H
#define __KOS_CDEFS_H

#include <sys/cdefs.h>

/* Check GCC version */
#if __GNUC__ <= 3
#   warning Your GCC is too old. This will probably not work right.
#endif

/** \defgroup system_attributes Function Attribute Defines
    \brief                      Definitions for builtin attributes and compiler directives
    \ingroup                    system

    This group contains definitions of various __attribute__ directives in
    shorter forms for use in programs. These typically aid  in optimizations
    or provide the compiler with extra information about a symbol.

    @{
*/

#ifndef __noreturn
/** \brief  Identify a function that will never return. */
#define __noreturn  __attribute__((__noreturn__))
#endif

#ifndef __unused
/** \brief  Identify a function or variable that may be unused. */
#define __unused    __attribute__((__unused__))
#endif

#ifndef __used
/** \brief  Prevent a symbol from being removed from the binary. */
#define __used      __attribute__((used))
#endif

#ifndef __weak
/** \brief  Identify a function or variable that may be overridden by another symbol. */
#define __weak      __attribute__((weak))
#endif

#ifndef __packed
/** \brief  Force a structure, enum, or other type to be packed as small as possible. */
#define __packed    __attribute__((packed))
#endif

#ifndef __dead2
/** \brief  Alias for \ref __noreturn. For BSD compatibility. */
#define __dead2     __noreturn  /* BSD compat */
#endif

#ifndef __likely
/** \brief  Directive to inform the compiler the condition is in the likely path.

    This can be used around conditionals or loops to help inform the
    compiler which path to optimize for as the common-case.

    \param  exp     Boolean expression which expected to be true.

    \sa __unlikely()
*/
#define __likely(exp)   __builtin_expect(!!(exp), 1)
#endif

#ifndef __unlikely
/** \brief  Directive to inform the compiler the condition is in the unlikely path.

    This can be used around conditionals or loops to help inform the
    compiler which path to optimize against as the infrequent-case.

    \param  exp     Boolean expression which is expected to be false.

    \sa __likely()
*/
#define __unlikely(exp) __builtin_expect(!!(exp), 0)
#endif

#ifndef __deprecated
/** \brief  Mark something as deprecated.
    This should be used to warn users that a function/type/etc will be removed
    in a future version of KOS. */
#define __deprecated    __attribute__((deprecated))
#endif

#ifndef __depr
/** \brief  Mark something as deprecated, with an informative message.
    This should be used to warn users that a function/type/etc will be removed
    in a future version of KOS and to suggest an alternative that they can use
    instead.
    \param  m       A string literal that is included with the warning message
                    at compile time. */
#define __depr(m) __attribute__((deprecated(m)))
#endif

/* Printf/Scanf-like declaration */
#ifndef __printflike
/** \brief  Identify a function as accepting formatting like printf().

    Using this macro allows GCC to typecheck calls to printf-like functions,
    which can aid in finding mistakes.

    \param  fmtarg          The argument number (1-based) of the format string.
    \param  firstvararg     The argument number of the first vararg (the ...).
*/
#define __printflike(fmtarg, firstvararg) \
    __attribute__((__format__ (__printf__, fmtarg, firstvararg)))
#endif

#ifndef __scanflike
/** \brief  Identify a function as accepting formatting like scanf().

    Using this macro allows GCC to typecheck calls to scanf-like functions,
    which can aid in finding mistakes.

    \param  fmtarg          The argument number (1-based) of the format string.
    \param  firstvararg     The argument number of the first vararg (the ...).
*/
#define __scanflike(fmtarg, firstvararg) \
    __attribute__((__format__ (__scanf__, fmtarg, firstvararg)))
#endif

#if __GNUC__ >= 7
/** \brief  Identify a case statement that is expected to fall through to the
            statement underneath it. */
#define __fallthrough __attribute__((__fallthrough__))
#else
#define __fallthrough /* Fall through */
#endif

#ifndef __always_inline
/** \brief  Ask the compiler to \a always inline a given function. */
#define __always_inline inline __attribute__((__always_inline__))
#endif

#ifndef __no_inline
/** \brief Ask the compiler to \a never inline a given function. */
#define __no_inline __attribute__((__noinline__))
#endif

/** @} */

/** \defgroup system_compat Language Compatibility Defines
    \brief                      Definitions for language features
    \ingroup                    system

    This group contains definitions to help retain some older language
    backwards compatibility for external software linking into KOS.

    @{
*/

/* GCC macros for special cases */
/* #if __GNUC__ ==  */

#ifndef __RESTRICT
#if (__STDC_VERSION__ >= 199901L)
#define __RESTRICT restrict
#elif defined(__GNUC__) || defined(__GNUG__)
#define __RESTRICT __restrict__
#else /* < C99 and not GCC */
#define __RESTRICT
#endif
#endif /* !__RESTRICT */

#ifndef __GNUC__
#define __extension__
#endif

#ifndef __GNUC_STDC_INLINE__
#define inline __inline__
#endif

/** @} */

/** \defgroup system_helpers Helper Macros
    \brief                      General useful language macros
    \ingroup                    system

    This group contains definitions to help give robust solutions
    to common code patterns.

    @{
*/

/** \brief Assert a build-time dependency.

    Your compiler will fail if the condition isn't true, or can't be evaluated
    by the compiler. This can only be used within a function.

    Example:
    \#include <stddef.h>
    ...
    static char *foo_to_char(struct foo *foo)
    {
        // This code needs string to be at start of foo.
        __build_assert(offsetof(struct foo, string) == 0);
        return (char *)foo;
    }

    \param cond     The compile-time condition which must be true.

    \sa __build_assert_or_zero
 */
#define __build_assert(cond) \
    do { (void) sizeof(char [1 - 2*!(cond)]); } while(0)

/** \brief Assert a build-time dependency.

    Your compiler will fail if the condition isn't true, or can't be evaluated
    by the compiler. This can be used in an expression: its value is "0".

    Example:
    \#define foo_to_char(foo)                \
        ((char *)(foo)                      \
        + __build_assert_or_zero(offsetof(struct foo, string) == 0))

    \param cond     The compile-time condition which must be true.

    \sa __build_assert
 */
#define __build_assert_or_zero(cond) \
    (sizeof(char [1 - 2*!(cond)]) - 1)

/** \brief Get the number of elements in a visible array.

    This does not work on pointers, or arrays declared as [], or
    function parameters. With correct compiler support, such usage
    will cause a build error (\see __build_assert).

    \param arr      The array whose size you want.

 */
#define __array_size(arr) (sizeof(arr) / sizeof((arr)[0]) + _array_size_chk(arr))

/* Helper for __array_size's type check */
#if HAVE_BUILTIN_TYPES_COMPATIBLE_P && HAVE_TYPEOF
/* Two gcc extensions.
 * &a[0] degrades to a pointer: a different type from an array */
#define _array_size_chk(arr)                        \
    __build_assert_or_zero(!__builtin_types_compatible_p(typeof(arr),   \
                            typeof(&(arr)[0])))
#else
#define _array_size_chk(arr) 0
#endif

/** \brief Create a string from the argument.

    \param arg      The text to stringify.
 */
#define __stringify(arg) ""#arg

/** \brief Check if a macro is defined to 1.

    \param macro    The macro to check
    \return         1 if the macro is defined to 1, 0 otherwise.
 */
#define __is_defined(macro) !__builtin_strcmp(__stringify(macro), "1")

/** @} */

#endif  /* __KOS_CDEFS_H */
