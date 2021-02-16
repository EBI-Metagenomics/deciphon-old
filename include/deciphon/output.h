#ifndef DECIPHON_OUTPUT_H
#define DECIPHON_OUTPUT_H

#include "deciphon/export.h"
#include "nmm/nmm.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

struct dcp_output;

DCP_API struct dcp_output* dcp_output_create(char const* filepath, uint32_t nmodels);
DCP_API int dcp_output_write(struct dcp_output* output, struct nmm_model const* model);
DCP_API int dcp_output_destroy(struct dcp_output* output);
DCP_API int dcp_output_close(struct dcp_output* output);

#endif
