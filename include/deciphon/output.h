#ifndef DECIPHON_OUTPUT_H
#define DECIPHON_OUTPUT_H

#include "deciphon/export.h"
#include <inttypes.h>

struct dcp_output;
struct nmm_profile;

DCP_API struct dcp_output* dcp_output_create(char const* filepath, uint32_t nmodels);
DCP_API int                dcp_output_write(struct dcp_output* output, struct nmm_profile const* prof);
DCP_API int                dcp_output_destroy(struct dcp_output* output);
DCP_API int                dcp_output_close(struct dcp_output* output);

#endif
