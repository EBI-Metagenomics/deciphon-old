#ifndef DCP_RESULTS_H
#define DCP_RESULTS_H

#include "dcp/export.h"
#include <stdint.h>

struct dcp_result;
struct dcp_results;

DCP_API struct dcp_result const* dcp_results_get(struct dcp_results* results, uint16_t i);
DCP_API uint16_t                 dcp_results_size(struct dcp_results const* results);

#endif
