#ifndef FLS_H
#define FLS_H

#include <stdint.h>

/**
 * fls32 - find last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note fls32(1) = 1, fls32(0x80000000) = 32.
 *
 * Undefined if no set bit exists, so code should check against 0 first.
 */
static inline unsigned fls32(uint32_t x)
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

/**
 * fls64 - find last (most-significant) set bit in a long word
 * @x: the word to search
 *
 * Undefined if no set bit exists, so code should check against 0 first.
 */
static inline unsigned fls64(uint64_t x)
{

#if __has_builtin(__builtin_clzl)
    uint32_t h = x >> 32;
    return h ? fls32(h) + 32 : fls32((uint32_t)x);
#else
    return (unsigned)((int)sizeof(long) * 8 - __builtin_clzl(x));
#endif
}

#endif
