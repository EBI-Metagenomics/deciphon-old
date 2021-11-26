#ifndef XMATH_H
#define XMATH_H

#include <assert.h>
#include <limits.h>

static inline unsigned xmath_min(unsigned a, unsigned b)
{
    return a < b ? a : b;
}
static inline unsigned xmath_max(unsigned a, unsigned b)
{
    return a > b ? a : b;
}

static inline unsigned xmath_ceildiv(unsigned x, unsigned y)
{
    assert(y > 0);
    assert(y - 1 <= UINT_MAX - x);
    return (x + y - 1) / y;
}

static unsigned xmath_partition_size(unsigned nelems, unsigned nparts,
                                     unsigned idx)
{
    unsigned size = xmath_ceildiv(nelems, nparts);
    assert(nelems >= size * idx);
    return xmath_min(size, nelems - size * idx);
}

#endif
