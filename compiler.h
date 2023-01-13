#ifndef COMPILER_H
#define COMPILER_H

#include "bug.h"

/*
 * __has_builtin is supported on gcc >= 10, clang >= 3 and icc >= 21.
 * In the meantime, to support gcc < 10, we implement __has_builtin
 * by hand.
 */
#ifndef __has_builtin
#define __has_builtin(x) (0)
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

#endif
