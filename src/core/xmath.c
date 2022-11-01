#include <assert.h>
#include <limits.h>

long xmath_min(long a, long b) { return a < b ? a : b; }

long xmath_max(long a, long b) { return a > b ? a : b; }

long xmath_ceildiv(long x, long y)
{
    assert(y > 0);
    assert(y - 1 <= UINT_MAX - x);
    return (x + y - 1) / y;
}

long xmath_partition_size(long nelems, long nparts, long idx)
{
    return xmath_ceildiv(xmath_max(0, nelems - idx), nparts);
}

float xmath_lrt_f32(float null_loglik, float alt_loglik)
{
    return -2 * (null_loglik - alt_loglik);
}

double xmath_lrt_f64(double null_loglik, double alt_loglik)
{
    return -2 * (null_loglik - alt_loglik);
}
