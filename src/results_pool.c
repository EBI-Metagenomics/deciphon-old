#include "results_pool.h"
#include "deciphon/deciphon.h"
#include "results.h"
#include "util.h"
#include <stdlib.h>

struct results_pool
{
    unsigned             size;
    struct dcp_results** results;
    unsigned             curr;
    unsigned             navail;
};

struct results_pool* results_pool_create(unsigned size)
{
    struct results_pool* pool = malloc(sizeof(*pool));
    pool->size = size;
    pool->results = malloc(sizeof(*pool->results) * size);
    for (unsigned i = 0; i < size; ++i)
        pool->results[i] = results_create();
    pool->curr = size - 1;
    pool->navail = size;
    return pool;
}

void results_pool_destroy(struct results_pool const* pool)
{
    BUG(pool->navail != pool->size);
    for (unsigned i = 0; i < pool->size; ++i)
        results_destroy(pool->results[i]);
    free(pool->results);
    free((void*)pool);
}

struct dcp_results* results_pool_get(struct results_pool* pool)
{
    struct dcp_results* results = NULL;

#pragma omp critical
    if (pool->navail > 0) {

        results = pool->results[pool->curr];
        pool->curr = ((pool->curr + pool->size) - 1) % pool->size;
        pool->navail--;
    }

    return results;
}

void results_pool_add(struct results_pool* pool, struct dcp_results* results)
{
#pragma omp critical
    {
        BUG(pool->navail == pool->size);
        pool->curr = (pool->curr + 1) % pool->size;
        pool->navail++;
        pool->results[pool->curr] = results;
    }
}
