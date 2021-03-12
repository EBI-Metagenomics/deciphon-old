#ifndef DECIPHON_RESULTS_H
#define DECIPHON_RESULTS_H

#include "deciphon/export.h"

struct dcp_result;
struct dcp_results;

DCP_API void                     dcp_results_destroy(struct dcp_results const* results);
DCP_API struct dcp_result const* dcp_results_first(struct dcp_results const* results);
DCP_API struct dcp_result const* dcp_results_next(struct dcp_results const* results, struct dcp_result const* result);

#endif
