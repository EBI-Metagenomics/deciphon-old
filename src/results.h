#ifndef RESULTS_H
#define RESULTS_H

#include "llist.h"
#include "result.h"
#include <stdint.h>

struct dcp_results;

#define RESULTS_BUFFSIZE 4
/* #define RESULTS_BUFFSIZE 1024 */

struct dcp_results
{
    struct llist_node node;
    struct llist_node link;
    uint16_t          nresults;
    uint16_t          curr;
    struct dcp_result results[RESULTS_BUFFSIZE];
};

void               results_deinit(struct dcp_results* results);
void               results_init(struct dcp_results* results);
uint16_t           results_limit(struct dcp_results const* results);
struct dcp_result* results_next(struct dcp_results* results);
void               results_rewind(struct dcp_results* results);

#endif
