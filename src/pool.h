#ifndef POOL_H
#define POOL_H

#include "bitops.h"
#include "size.h"
#include <limits.h>
#include <stdbool.h>

#define POOL_SIZE SIZE_BITS_PER_INT

struct pool
{
    unsigned long avail;
    unsigned *ids[POOL_SIZE];
};

void pool_init(struct pool *pool);
void pool_assoc(struct pool *pool, unsigned *id);
unsigned *pool_pop(struct pool *pool);

static inline void pool_put(struct pool *pool, unsigned *id)
{
    bitops_set(*id, &pool->avail);
}

static inline bool pool_full(struct pool *pool)
{
    return pool->avail == UINT_MAX;
}

static inline bool pool_empty(struct pool *pool) { return pool->avail == 0; }

#endif
