#ifndef RESULTS_POOL_H
#define RESULTS_POOL_H

#include <stdbool.h>

struct dcp_results;
struct results_pool;

struct results_pool* results_pool_create(unsigned size);
void                 results_pool_destroy(struct results_pool const* pool);
struct dcp_results*  results_pool_get(struct results_pool* pool);
void                 results_pool_add(struct results_pool* pool, struct dcp_results* results);

#endif
