#ifndef LRT_H
#define LRT_H

float lrt32(float null_loglik, float alt_loglik);
double lrt64(double null_loglik, double alt_loglik);

#define lrt(null, alt) _Generic((null), float: lrt32, double: lrt64)(null, alt)

#endif
