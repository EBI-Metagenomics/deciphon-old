#ifndef RESULTS_H
#define RESULTS_H

#include "containers/snode.h"
#include "result.h"
#include <stdint.h>

struct dcp_results;

#define RESULTS_BUFFSIZE 512

struct dcp_results
{
    struct snode      node;
    uint16_t          size;
    struct dcp_result data[RESULTS_BUFFSIZE];
};

void               results_deinit(struct dcp_results const* results);
static inline bool results_empty(struct dcp_results const* results) { return results->size == 0; }
static inline bool results_full(struct dcp_results const* results) { return results->size == RESULTS_BUFFSIZE; }
void               results_init(struct dcp_results* results);
static inline struct dcp_result* results_pop(struct dcp_results* results) { return results->data + --results->size; }
static inline struct dcp_result* results_put(struct dcp_results* results) { return results->data + results->size++; }
static inline void               results_reset(struct dcp_results* results) { results->size = 0; }

#endif
