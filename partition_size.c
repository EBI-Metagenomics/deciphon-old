#include "partition_size.h"
#include <assert.h>
#include <limits.h>

static inline long max(long a, long b) { return a > b ? a : b; }

static inline long ceildiv(long x, long y)
{
  assert(y > 0);
  assert(y - 1 <= UINT_MAX - x);
  return (x + y - 1) / y;
}

long partition_size(long nelems, long nparts, long idx)
{
  return ceildiv(max(0, nelems - idx), nparts);
}
