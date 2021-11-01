#ifndef BITS_H
#define BITS_H

#include <stdbool.h>
#include <stdint.h>

#define __BITS_DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))

#define BITS_PER_BYTE 8
#define BITS_TO_LONGS(nr) __BITS_DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))

static inline void bits_clr(unsigned *x, unsigned bit)
{
    *x &= ~(1U << bit);
}

static inline bool bits_get(unsigned *x, unsigned bit)
{
    return !!((*x >> bit) & 1U);
}

static inline void bits_set(unsigned *x, unsigned bit)
{
    *x |= 1U << bit;
}

static inline unsigned bits_width(uint32_t v)
{
    return v ? ((unsigned)__builtin_clz(v) ^ 31) + 1 : 0;
}

#endif
