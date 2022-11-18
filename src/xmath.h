#ifndef CORE_XMATH_H
#define CORE_XMATH_H

long xmath_min(long a, long b);
long xmath_max(long a, long b);
long xmath_ceildiv(long x, long y);
long xmath_partition_size(long nelems, long nparts, long idx);
float xmath_lrt_f32(float null_loglik, float alt_loglik);
double xmath_lrt_f64(double null_loglik, double alt_loglik);

#define xmath_lrt(null, alt)                                                   \
    _Generic((null), float : xmath_lrt_f32, double : xmath_lrt_f64)(null, alt)

#endif
