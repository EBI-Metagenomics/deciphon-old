#include "results.h"
#include "dcp/dcp.h"
#include "result.h"

void results_deinit(struct dcp_results const* results)
{
    for (unsigned i = 0; i < RESULTS_BUFFSIZE; ++i)
        result_deinit(results->data + i);
}

void results_init(struct dcp_results* results)
{
    snode_init(&results->node);
    results->size = 0;
    for (unsigned i = 0; i < RESULTS_BUFFSIZE; ++i)
        result_init(results->data + i, results);
}
