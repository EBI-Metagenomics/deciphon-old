#ifndef DECIPHON_COMPILER_H
#define DECIPHON_COMPILER_H

#include "deciphon/core/bug.h"

/*
 * __has_builtin is supported on gcc >= 10, clang >= 3 and icc >= 21.
 * In the meantime, to support gcc < 10, we implement __has_builtin
 * by hand.
 */
#ifndef __has_builtin
#define __has_builtin(x) (0)
#endif

#if !__has_builtin(__builtin_unreachable)
#define __builtin_unreachable() (void)(0)
#endif

/* clang-format off */
#define BITS_PER(x) (\
    (sizeof(x) == 8 ? 64U :\
    (sizeof(x) == 7 ? 48U :\
    (sizeof(x) == 6 ? 56U :\
    (sizeof(x) == 5 ? 40U :\
    (sizeof(x) == 4 ? 32U :\
    (sizeof(x) == 3 ? 24U :\
    (sizeof(x) == 2 ? 16U :\
    (sizeof(x) == 1 ? 8U : BUILD_BUG_ON_ZERO(0))))))))))
/* clang-format on */

#define TRUE (!!1)
#define FALSE (!!0)

/*
 * Evaluates to `ONE` in case of `EXPR` being `true`; `TWO` otherwise.
 *
 * Acknowledgement: Jens Gustedt.
 */
#define BUILD_SWITCH(EXPR, ONE, TWO)                                           \
    _Generic((1 ? (struct p00_nullptr_test *)0 : (void *)!(EXPR)),             \
             struct p00_nullptr_test *                                         \
             : ONE, default                                                    \
             : TWO)

#define __MEMBER_REF(var, member) ((__typeof__(var) *)0)->member

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MEMBER_SIZE(var, member) sizeof(__MEMBER_REF((var), member))
#define ARRAY_SIZE_OF(var, member) ARRAY_SIZE(__MEMBER_REF((var), member))

/*
 * String related macros.
 */
#define STRINGIFY(s) __STRINGIFY(s)
#define __STRINGIFY(s) #s

#ifdef __FILE_NAME__
#define LOCAL __FILE_NAME__ ":" STRINGIFY(__LINE__)
#else
#define LOCAL __FILE__ ":" STRINGIFY(__LINE__)
#endif

/* Are two types/vars the same type (ignoring qualifiers)? */
#define SAME_TYPE(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))

#endif
