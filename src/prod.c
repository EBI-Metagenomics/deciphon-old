#include "dcp/prod.h"
#include "xmath.h"

double dcp_prod_lrt(struct dcp_prod const *prod)
{
    return xmath_lrt(prod->null_loglik, prod->loglik);
}
