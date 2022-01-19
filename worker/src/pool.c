#include "pool.h"
#include <string.h>

void pool_init(struct pool *pool) { memset(pool, 0, sizeof(*pool)); }

void pool_assoc(struct pool *pool, unsigned *id)
{
    int i = bitops_fful(pool->avail ^ UINT_MAX);
    bitops_set((unsigned)i, &pool->avail);
    *id = (unsigned)i;
    pool->ids[i] = id;
}

unsigned *pool_pop(struct pool *pool)
{
    int i = bitops_fful(pool->avail);
    bitops_clr((unsigned)i, &pool->avail);
    return pool->ids[i];
}