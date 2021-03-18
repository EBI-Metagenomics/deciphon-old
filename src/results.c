#include "results.h"
#include "dcp/dcp.h"
#include "result.h"

void results_deinit(struct dcp_results* results) { free((void*)results); }

struct dcp_result const* dcp_results_get(struct dcp_results* results, uint16_t i) { return results->results + i; }

uint16_t dcp_results_size(struct dcp_results const* results) { return results->curr; }

void results_init(struct dcp_results* results)
{
    llist_init_node(&results->node);
    llist_init_node(&results->link);
    results->nresults = RESULTS_BUFFSIZE;
    for (unsigned i = 0; i < RESULTS_BUFFSIZE; ++i)
        result_init(results->results + i);
    results->curr = 0;
}

uint16_t results_limit(struct dcp_results const* results) { return RESULTS_BUFFSIZE; }

struct dcp_result* results_next(struct dcp_results* results)
{
    if (results->curr == results->nresults)
        return NULL;
    return &results->results[results->curr++];
}

void results_rewind(struct dcp_results* results) { results->curr = 0; }
