#ifndef ILOG_H
#define ILOG_H

#include <stdint.h>

/*
 * __has_builtin is supported on gcc >= 10, clang >= 3 and icc >= 21.
 * In the meantime, to support gcc < 10, we implement __has_builtin
 * by hand.
 */
#ifndef __has_builtin
#define __has_builtin(x) (0)
#endif

/**
 * __dcp_fls32 - find last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note __dcp_fls32(1) = 1, __dcp_fls32(0x80000000) = 32.
 *
 * Undefined if no set bit exists, so code should check against 0 first.
 */
static inline unsigned __dcp_fls32(uint32_t x)
{
#if __has_builtin(__builtin_clz)
    return (unsigned)((int)sizeof(int) * 8 - __builtin_clz(x));
#else
    unsigned r = 32;

    if (!(x & 0xffff0000u))
    {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u))
    {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u))
    {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u))
    {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u))
    {
        x <<= 1;
        r -= 1;
    }
    return r;
#endif
}

static inline unsigned ilog2(uint32_t n) { return __dcp_fls32(n) - 1; }

#endif
