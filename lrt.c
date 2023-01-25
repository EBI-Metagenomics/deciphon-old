#include "lrt.h"

float lrt32(float null_loglik, float alt_loglik)
{
  return -2 * (null_loglik - alt_loglik);
}

double lrt64(double null_loglik, double alt_loglik)
{
  return -2 * (null_loglik - alt_loglik);
}
