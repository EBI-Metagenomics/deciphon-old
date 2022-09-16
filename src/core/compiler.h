#ifndef CORE_COMPILER_H
#define CORE_COMPILER_H

#include "core/bug.h"

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

/* Are two types/vars the same type (ignoring qualifiers)? */
#define SAME_TYPE(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))

// Source: https://stackoverflow.com/a/18298965
#ifndef thread_local
#if __STDC_VERSION__ >= 201112 && !defined __STDC_NO_THREADS__
#define thread_local _Thread_local
#elif defined _WIN32 && (defined _MSC_VER || defined __ICL ||                  \
                         defined __DMC__ || defined __BORLANDC__)
#define thread_local __declspec(thread)
/* note that ICC (linux) and Clang are covered by __GNUC__ */
#elif defined __GNUC__ || defined __SUNPRO_C || defined __xlC__
#define thread_local __thread
#else
#error "Cannot define thread_local"
#endif
#endif

#endif