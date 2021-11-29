#ifndef BITOPS_H
#define BITOPS_H

#include "size.h"
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
 * fls32 - find last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note fls32(1) = 1, fls32(0x80000000) = 32.
 *
 * Undefined if no set bit exists, so code should check against 0 first.
 */
static inline unsigned bitops_fls32(uint32_t x)
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
static inline unsigned bitops_fls64(uint64_t x)
{

#if __has_builtin(__builtin_clzl)
    uint32_t h = (uint32_t)(x >> 32);
    return h ? bitops_fls32(h) + 32 : bitops_fls32((uint32_t)x);
#else
    return (unsigned)((int)sizeof(long) * 8 - __builtin_clzl(x));
#endif
}

/**
 * bitops_ffs - find first bit in word.
 * @x: The word to search
 *
 * Undefined if no bit exists, so code should check against 0 first.
 */
static inline int bitops_fful(unsigned long word)
{
#if __has_builtin(__builtin_ffsl)
    return __builtin_ffsl((long)word) - 1;
#else
    int num = 0;

#if BITS_PER_LONG == 64
    if ((word & 0xffffffff) == 0)
    {
        num += 32;
        word >>= 32;
    }
#endif
    if ((word & 0xffff) == 0)
    {
        num += 16;
        word >>= 16;
    }
    if ((word & 0xff) == 0)
    {
        num += 8;
        word >>= 8;
    }
    if ((word & 0xf) == 0)
    {
        num += 4;
        word >>= 4;
    }
    if ((word & 0x3) == 0)
    {
        num += 2;
        word >>= 2;
    }
    if ((word & 0x1) == 0) num += 1;
    return num;
#endif
}

#define BITOPS_MASK(nr) (1UL << ((nr) % SIZE_BITS_PER_LONG))
#define BITOPS_WORD(nr) ((nr) / SIZE_BITS_PER_LONG)

static inline void bitops_set(unsigned nr, volatile unsigned long *addr)
{
    unsigned long mask = BITOPS_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + BITOPS_WORD(nr);

    *p |= mask;
}

static inline void bitops_clr(unsigned nr, volatile unsigned long *addr)
{
    unsigned long mask = BITOPS_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + BITOPS_WORD(nr);

    *p &= ~mask;
}

#endif
