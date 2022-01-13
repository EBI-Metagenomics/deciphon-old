#ifndef BITOPS_H
#define BITOPS_H

#include "common/compiler.h"
#include "common/export.h"
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
EXPORT unsigned bitops_fls32(uint32_t x);

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
EXPORT int bitops_fful(unsigned long word);

#define BITOPS_MASK(nr) (1UL << ((nr) % BITS_PER(long)))
#define BITOPS_WORD(nr) ((nr) / BITS_PER(long))

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
