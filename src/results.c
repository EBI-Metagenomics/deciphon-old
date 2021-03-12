#include "results.h"
#include "deciphon/deciphon.h"
#include "result.h"
#include <stdlib.h>

struct dcp_results
{
    struct list results;
};

struct dcp_results* results_create(void)
{
    struct dcp_results* results = malloc(sizeof(*results));
    list_init(&results->results);
    return results;
}

void dcp_results_destroy(struct dcp_results const* results) { free((void*)results); }

struct dcp_result const* dcp_results_first(struct dcp_results const* results)
{
    return container_of(list_head(&results->results), struct dcp_result, link);
}

struct dcp_result const* dcp_results_next(struct dcp_results const* results, struct dcp_result const* result)
{
    struct list* i = list_next(&results->results, &result->link);
    return i ? container_of(i, struct dcp_result, link) : NULL;
}

void results_add(struct dcp_results* results, struct dcp_result* result) { list_add(&results->results, &result->link); }
