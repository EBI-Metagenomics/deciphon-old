#include "results.h"
#include "deciphon/deciphon.h"
#include "lib/list.h"
#include "result.h"
#include "results_pool.h"
#include <stdlib.h>

void results_destroy(struct dcp_results const* results) { free((void*)results); }

struct dcp_result const* dcp_results_get(struct dcp_results* results, uint16_t i) { return results->results[i]; }

void dcp_results_release(struct dcp_results* results) { results_pool_add(results->pool, results); }

uint16_t dcp_results_size(struct dcp_results const* results) { return results->nresults; }

struct dcp_results* results_create(void)
{
    struct dcp_results* results = malloc(sizeof(*results));
    list_init(&results->link);
    results->nresults = 0;
    for (uint16_t i = 0; i < RESULTS_BUFFSIZE; ++i)
        results->results[i] = result_create();
    return results;
}

struct dcp_result* results_get(struct dcp_results* results, uint16_t i)
{
    results->nresults = i + 1;
    return results->results[i];
}

uint16_t results_limit(struct dcp_results const* results) { return RESULTS_BUFFSIZE; }
