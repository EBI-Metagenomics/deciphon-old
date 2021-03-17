#ifndef RESULTS_H
#define RESULTS_H

#include "lib/list.h"
#include <stdint.h>

struct dcp_result;
struct dcp_results;
struct results_pool;

#define RESULTS_BUFFSIZE 1024

struct dcp_results
{
    struct results_pool* pool;
    struct list          link;
    uint16_t             nresults;
    struct dcp_result*   results[RESULTS_BUFFSIZE];
};

struct dcp_results* results_create(void);
void                results_destroy(struct dcp_results const* results);
struct dcp_result*  results_get(struct dcp_results* results, uint16_t i);
uint16_t            results_limit(struct dcp_results const* results);

#endif
