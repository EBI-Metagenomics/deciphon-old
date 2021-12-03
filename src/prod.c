#include "prod.h"
#include "xmath.h"

double prod_lrt(struct prod const *prod)
{
    return xmath_lrt(prod->null_loglik, prod->loglik);
}
