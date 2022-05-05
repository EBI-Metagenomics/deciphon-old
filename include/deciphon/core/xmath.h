#ifndef DECIPHON_XMATH_H
#define DECIPHON_XMATH_H

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

static inline unsigned xmath_partition_size(unsigned nelems, unsigned nparts,
                                            unsigned idx)
{
    unsigned size = xmath_ceildiv(nelems, nparts);
    assert(nelems >= size * idx);
    return xmath_min(size, nelems - size * idx);
}

static inline float xmath_lrt_f32(float null_loglik, float alt_loglik)
{
    return -2 * (null_loglik - alt_loglik);
}

static inline double xmath_lrt_f64(double null_loglik, double alt_loglik)
{
    return -2 * (null_loglik - alt_loglik);
}

#define xmath_lrt(null, alt)                                                   \
    _Generic((null), float : xmath_lrt_f32, double : xmath_lrt_f64)(null, alt)

#endif
