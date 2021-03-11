#include "results.h"
#include "deciphon/deciphon.h"
#include "lib/c-list.h"
#include "result.h"
#include <stdlib.h>

struct dcp_results
{
    CList results;
};

struct dcp_results* dcp_results_create(void)
{
    struct dcp_results* results = malloc(sizeof(*results));
    c_list_init(&results->results);
    return results;
}

void dcp_results_destroy(struct dcp_results const* results) { free((void*)results); }

struct dcp_result const* dcp_results_first(struct dcp_results const* results)
{
    return c_list_first_entry(&results->results, struct dcp_result const, link);
}

struct dcp_result const* dcp_results_next(struct dcp_results const* results, struct dcp_result const* result)
{
    CList const* curr = &result->link;
    CList const* next = curr->next;
    if (next == &results->results)
        return NULL;
    return c_list_entry(next, struct dcp_result const, link);
}

void results_add(struct dcp_results* results, struct dcp_result* result)
{
    c_list_link_tail(&results->results, &result->link);
}
